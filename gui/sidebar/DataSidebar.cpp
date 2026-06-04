#include "DataSidebar.h"
#include "../teachers/TeacherDialog.h"
#include "../subjects/SubjectDialog.h"
#include "../classes/ClassDialog.h"
#include "../rooms/RoomDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>

DataSidebar::DataSidebar(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setupUi();
    populateTree();
}

void DataSidebar::setupUi() {
    setObjectName("DataSidebar");
    setMinimumWidth(200);
    setMaximumWidth(320);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QLabel *title = new QLabel(tr("  Resources"), this);
    title->setObjectName("SidebarTitle");
    title->setFixedHeight(36);
    mainLayout->addWidget(title);

    tree = new QTreeWidget(this);
    tree->setObjectName("SidebarTree");
    tree->setHeaderHidden(true);
    tree->setRootIsDecorated(true);
    tree->setAnimated(true);
    tree->setIndentation(16);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(tree, 1);

    // Quick-add buttons at bottom
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setContentsMargins(6, 4, 6, 4);
    btnLayout->setSpacing(4);

    addTeacherBtn = new QPushButton(tr("+ Teacher"), this);
    addSubjectBtn = new QPushButton(tr("+ Subject"), this);
    addClassBtn = new QPushButton(tr("+ Class"), this);
    addRoomBtn = new QPushButton(tr("+ Room"), this);

    for (auto *btn : {addTeacherBtn, addSubjectBtn, addClassBtn, addRoomBtn}) {
        btn->setFixedHeight(26);
        btn->setStyleSheet(
            "QPushButton { background: #2a2a2a; border: 1px solid #444; "
            "border-radius: 3px; padding: 2px 8px; color: #ccc; font-size: 8pt; }"
            "QPushButton:hover { background: #ff6600; color: #fff; }"
        );
    }

    btnLayout->addWidget(addTeacherBtn);
    btnLayout->addWidget(addSubjectBtn);
    btnLayout->addWidget(addClassBtn);
    btnLayout->addWidget(addRoomBtn);
    mainLayout->addLayout(btnLayout);

    // Connections
    connect(tree, &QTreeWidget::itemClicked, this, &DataSidebar::onItemClicked);
    connect(tree, &QTreeWidget::itemDoubleClicked, this, &DataSidebar::onItemDoubleClicked);
    connect(tree, &QTreeWidget::customContextMenuRequested, this, &DataSidebar::onContextMenu);

    connect(addTeacherBtn, &QPushButton::clicked, this, [this]() { openAddDialog("teacher"); });
    connect(addSubjectBtn, &QPushButton::clicked, this, [this]() { openAddDialog("subject"); });
    connect(addClassBtn, &QPushButton::clicked, this, [this]() { openAddDialog("class"); });
    connect(addRoomBtn, &QPushButton::clicked, this, [this]() { openAddDialog("room"); });
}

void DataSidebar::populateTree() {
    tree->clear();

    auto makeSection = [&](const QString &title, int count) -> QTreeWidgetItem* {
        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setText(0, QString("%1  (%2)").arg(title).arg(count));
        item->setFlags(item->flags() | Qt::ItemIsEnabled);
        item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        item->setExpanded(true);
        QFont f = item->font(0);
        f.setBold(true);
        f.setPointSize(9);
        item->setFont(0, f);
        item->setForeground(0, QColor("#ff6600"));
        return item;
    };

    // Teachers
    teacherRoot = makeSection(tr("Teachers"), static_cast<int>(dm->teachers.size()));
    for (const auto &t : dm->teachers) {
        QTreeWidgetItem *child = new QTreeWidgetItem;
        child->setText(0, QString::fromStdString(t.name));
        child->setData(0, Qt::UserRole, "teacher");
        child->setData(0, Qt::UserRole + 1, t.id);
        child->setForeground(0, QColor("#e0e0e0"));
        teacherRoot->addChild(child);
    }
    tree->addTopLevelItem(teacherRoot);

    // Subjects
    subjectRoot = makeSection(tr("Subjects"), static_cast<int>(dm->subjects.size()));
    for (const auto &s : dm->subjects) {
        QTreeWidgetItem *child = new QTreeWidgetItem;
        child->setText(0, QString::fromStdString(s.name));
        child->setData(0, Qt::UserRole, "subject");
        child->setData(0, Qt::UserRole + 1, s.id);
        child->setForeground(0, QColor("#e0e0e0"));
        subjectRoot->addChild(child);
    }
    tree->addTopLevelItem(subjectRoot);

    // Classes
    classRoot = makeSection(tr("Classes"), static_cast<int>(dm->classes.size()));
    for (const auto &c : dm->classes) {
        QTreeWidgetItem *child = new QTreeWidgetItem;
        child->setText(0, QString("%1  (%2 students)").arg(QString::fromStdString(c.name)).arg(c.studentCount));
        child->setData(0, Qt::UserRole, "class");
        child->setData(0, Qt::UserRole + 1, c.id);
        child->setForeground(0, QColor("#e0e0e0"));
        classRoot->addChild(child);
    }
    tree->addTopLevelItem(classRoot);

    // Rooms
    roomRoot = makeSection(tr("Rooms"), static_cast<int>(dm->rooms.size()));
    for (const auto &r : dm->rooms) {
        QTreeWidgetItem *child = new QTreeWidgetItem;
        child->setText(0, QString("%1  (cap: %2)").arg(QString::fromStdString(r.name)).arg(r.capacity));
        child->setData(0, Qt::UserRole, "room");
        child->setData(0, Qt::UserRole + 1, r.id);
        child->setForeground(0, QColor("#e0e0e0"));
        roomRoot->addChild(child);
    }
    tree->addTopLevelItem(roomRoot);
}

void DataSidebar::onItemClicked(QTreeWidgetItem *item, int) {
    if (!item->parent()) return; // section header
    QString type = item->data(0, Qt::UserRole).toString();
    int id = item->data(0, Qt::UserRole + 1).toInt();
    emit itemSelected(type, id);
}

void DataSidebar::onItemDoubleClicked(QTreeWidgetItem *item, int) {
    if (!item->parent()) return;
    QString type = item->data(0, Qt::UserRole).toString();
    int id = item->data(0, Qt::UserRole + 1).toInt();
    openEditDialog(type, id);
}

void DataSidebar::onContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = tree->itemAt(pos);
    if (!item || !item->parent()) return;
    QString type = item->data(0, Qt::UserRole).toString();
    int id = item->data(0, Qt::UserRole + 1).toInt();

    QMenu menu(this);
    QAction *editAction = menu.addAction(tr("Edit"));
    QAction *deleteAction = menu.addAction(tr("Delete"));
    QAction *chosen = menu.exec(tree->viewport()->mapToGlobal(pos));
    if (chosen == editAction) {
        openEditDialog(type, id);
    } else if (chosen == deleteAction) {
        deleteEntity(type, id);
    }
}

void DataSidebar::deleteEntity(const QString &type, int id) {
    QString name;
    if (type == "teacher") name = QString::fromStdString(dm->getTeacherName(id));
    else if (type == "subject") name = QString::fromStdString(dm->getSubjectName(id));
    else if (type == "class") name = QString::fromStdString(dm->getClassName(id));
    else if (type == "room") name = QString::fromStdString(dm->getRoomName(id));

    auto result = QMessageBox::question(this, tr("Confirm Delete"),
        tr("Delete %1 \"%2\" (ID: %3)?").arg(type).arg(name).arg(id));
    if (result != QMessageBox::Yes) return;

    bool ok = false;
    if (type == "teacher") ok = dm->removeTeacher(id);
    else if (type == "subject") ok = dm->removeSubject(id);
    else if (type == "class") ok = dm->removeClass(id);
    else if (type == "room") ok = dm->removeRoom(id);

    if (ok) {
        refresh();
    } else {
        QMessageBox::warning(this, tr("Delete Failed"), tr("Could not delete %1.").arg(type));
    }
}

void DataSidebar::openAddDialog(const QString &type) {
    if (type == "teacher") {
        TeacherDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            dm->addTeacher(dlg.teacherName().toStdString());
            refresh();
        }
    } else if (type == "subject") {
        SubjectDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            dm->addSubject(dlg.subjectName().toStdString());
            refresh();
        }
    } else if (type == "class") {
        ClassDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            dm->addClass(dlg.className().toStdString(), dlg.studentCount());
            refresh();
        }
    } else if (type == "room") {
        RoomDialog dlg(dm->roomTypes, this);
        if (dlg.exec() == QDialog::Accepted) {
            dm->addRoom(dlg.roomName().toStdString(), dlg.capacity(), dlg.roomTypeId());
            refresh();
        }
    }
}

void DataSidebar::openEditDialog(const QString &type, int id) {
    if (type == "teacher") {
        TeacherDialog dlg(this);
        dlg.setTeacherName(QString::fromStdString(dm->getTeacherName(id)));
        if (dlg.exec() == QDialog::Accepted) {
            dm->updateTeacher(id, dlg.teacherName().toStdString());
            refresh();
        }
    } else if (type == "subject") {
        SubjectDialog dlg(this);
        dlg.setSubjectName(QString::fromStdString(dm->getSubjectName(id)));
        if (dlg.exec() == QDialog::Accepted) {
            dm->updateSubject(id, dlg.subjectName().toStdString());
            refresh();
        }
    } else if (type == "class") {
        ClassDialog dlg(this);
        dlg.setClassName(QString::fromStdString(dm->getClassName(id)));
        dlg.setStudentCount(dm->classes[id - 1].studentCount);
        if (dlg.exec() == QDialog::Accepted) {
            dm->updateClass(id, dlg.className().toStdString(), dlg.studentCount());
            refresh();
        }
    } else if (type == "room") {
        RoomDialog dlg(dm->roomTypes, this);
        dlg.setRoomName(QString::fromStdString(dm->getRoomName(id)));
        dlg.setCapacity(dm->rooms[id - 1].capacity);
        dlg.setRoomTypeId(dm->rooms[id - 1].roomTypeId);
        if (dlg.exec() == QDialog::Accepted) {
            dm->updateRoom(id, dlg.roomName().toStdString(), dlg.capacity(), dlg.roomTypeId());
            refresh();
        }
    }
}

void DataSidebar::refresh() {
    populateTree();
}
