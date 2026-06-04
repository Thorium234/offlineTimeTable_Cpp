#include "RoomTypeWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QHeaderView>

RoomTypeWidget::RoomTypeWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Room Types & Requirements"));
    resize(800, 500);
    setupUi();
    connectSignals();
}

void RoomTypeWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QTabWidget *tabWidget = new QTabWidget(this);
    
    // Room Types tab
    QWidget *roomTypesTab = new QWidget(this);
    QVBoxLayout *roomTypesLayout = new QVBoxLayout(roomTypesTab);
    
    roomTypesTableView = new QTableView(roomTypesTab);
    roomTypesTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    roomTypesTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    roomTypesTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    roomTypesLayout->addWidget(roomTypesTableView);
    
    QHBoxLayout *roomTypesBtnLayout = new QHBoxLayout;
    roomTypeNameEdit = new QLineEdit(roomTypesTab);
    roomTypeNameEdit->setPlaceholderText(tr("Room Type Name (e.g., Lab, Gym)"));
    addRoomTypeBtn = new QPushButton(tr("Add Room Type"), roomTypesTab);
    delRoomTypeBtn = new QPushButton(tr("Delete"), roomTypesTab);
    roomTypesBtnLayout->addWidget(roomTypeNameEdit);
    roomTypesBtnLayout->addWidget(addRoomTypeBtn);
    roomTypesBtnLayout->addWidget(delRoomTypeBtn);
    roomTypesLayout->addLayout(roomTypesBtnLayout);
    
    // Subject Requirements tab
    QWidget *requirementsTab = new QWidget(this);
    QVBoxLayout *requirementsLayout = new QVBoxLayout(requirementsTab);
    
    requirementsTableView = new QTableView(requirementsTab);
    requirementsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    requirementsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    requirementsTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    requirementsLayout->addWidget(requirementsTableView);
    
    QHBoxLayout *reqSelectionLayout = new QHBoxLayout;
    reqSelectionLayout->addWidget(new QLabel(tr("Subject:"), requirementsTab));
    subjectCombo = new QComboBox(requirementsTab);
    reqSelectionLayout->addWidget(subjectCombo);
    reqSelectionLayout->addWidget(new QLabel(tr("Required Room Type:"), requirementsTab));
    roomTypeCombo = new QComboBox(requirementsTab);
    reqSelectionLayout->addWidget(roomTypeCombo);
    requirementsLayout->addLayout(reqSelectionLayout);
    
    QHBoxLayout *reqBtnLayout = new QHBoxLayout;
    addRequirementBtn = new QPushButton(tr("Set Requirement"), requirementsTab);
    delRequirementBtn = new QPushButton(tr("Remove"), requirementsTab);
    reqBtnLayout->addStretch();
    reqBtnLayout->addWidget(addRequirementBtn);
    reqBtnLayout->addWidget(delRequirementBtn);
    requirementsLayout->addLayout(reqBtnLayout);
    
    tabWidget->addTab(roomTypesTab, tr("Room Types"));
    tabWidget->addTab(requirementsTab, tr("Subject Requirements"));
    
    mainLayout->addWidget(tabWidget);
    
    setupRoomTypesTab();
    setupSubjectRequirementsTab();
}

void RoomTypeWidget::setupRoomTypesTab() {
    roomTypesModel = new QStandardItemModel(this);
    roomTypesModel->setColumnCount(2);
    roomTypesModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    roomTypesModel->setHeaderData(1, Qt::Horizontal, tr("Name"));
    
    for (const auto &rt : dm->roomTypes) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::number(rt.id));
        rowItems << new QStandardItem(QString::fromStdString(rt.name));
        roomTypesModel->appendRow(rowItems);
    }
    roomTypesTableView->setModel(roomTypesModel);
    roomTypesTableView->hideColumn(0);
    roomTypesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void RoomTypeWidget::setupSubjectRequirementsTab() {
    // Populate combos
    subjectCombo->clear();
    for (const auto &s : dm->subjects) {
        subjectCombo->addItem(QString::fromStdString(s.name), s.id);
    }
    
    roomTypeCombo->clear();
    for (const auto &rt : dm->roomTypes) {
        roomTypeCombo->addItem(QString::fromStdString(rt.name), rt.id);
    }
    
    // Setup table
    requirementsModel = new QStandardItemModel(this);
    requirementsModel->setColumnCount(3);
    requirementsModel->setHeaderData(0, Qt::Horizontal, tr("Subject"));
    requirementsModel->setHeaderData(1, Qt::Horizontal, tr("Required Room Type"));
    requirementsModel->setHeaderData(2, Qt::Horizontal, tr("ID"));
    
    for (const auto &sr : dm->subjectRequirements) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::fromStdString(dm->getSubjectName(sr.subjectId)));
        rowItems << new QStandardItem(QString::fromStdString(dm->getRoomTypeName(sr.roomTypeId)));
        rowItems << new QStandardItem(QString("%1:%2").arg(sr.subjectId).arg(sr.roomTypeId));
        requirementsModel->appendRow(rowItems);
    }
    requirementsTableView->setModel(requirementsModel);
    requirementsTableView->hideColumn(2);
    requirementsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void RoomTypeWidget::connectSignals() {
    connect(addRoomTypeBtn, &QPushButton::clicked, this, &RoomTypeWidget::addRoomType);
    connect(delRoomTypeBtn, &QPushButton::clicked, this, &RoomTypeWidget::deleteRoomType);
    connect(addRequirementBtn, &QPushButton::clicked, this, &RoomTypeWidget::addSubjectRequirement);
    connect(delRequirementBtn, &QPushButton::clicked, this, &RoomTypeWidget::deleteSubjectRequirement);
}

void RoomTypeWidget::addRoomType() {
    QString name = roomTypeNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Please enter a room type name."));
        return;
    }
    dm->addRoomType(name.toStdString());
    roomTypeNameEdit->clear();
    setupRoomTypesTab();
    setupSubjectRequirementsTab();
}

void RoomTypeWidget::deleteRoomType() {
    QModelIndex cur = roomTypesTableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select Room Type"), tr("Please select a room type to delete."));
        return;
    }
    
    int row = cur.row();
    int id = roomTypesModel->item(row, 0)->text().toInt();
    
    if (QMessageBox::question(this, tr("Confirm Delete"),
            tr("Delete room type with ID %1?").arg(id)) == QMessageBox::Yes) {
        dm->roomTypes.erase(std::remove_if(dm->roomTypes.begin(), dm->roomTypes.end(),
            [id](const RoomType &rt) { return rt.id == id; }), dm->roomTypes.end());
        setupRoomTypesTab();
    }
}

void RoomTypeWidget::addSubjectRequirement() {
    if (subjectCombo->count() == 0 || roomTypeCombo->count() == 0) {
        QMessageBox::warning(this, tr("Cannot Add"),
            tr("Please ensure you have at least one subject and room type defined."));
        return;
    }
    
    int subjectId = subjectCombo->currentData().toInt();
    int roomTypeId = roomTypeCombo->currentData().toInt();
    
    dm->setSubjectRequirement(subjectId, roomTypeId);
    setupSubjectRequirementsTab();
}

void RoomTypeWidget::deleteSubjectRequirement() {
    QModelIndex cur = requirementsTableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select Requirement"),
            tr("Please select a requirement to remove."));
        return;
    }
    
    int row = cur.row();
    QString idStr = requirementsModel->item(row, 2)->text();
    QStringList parts = idStr.split(':');
    
    if (parts.size() == 2) {
        int subjectId = parts[0].toInt();
        int roomTypeId = parts[1].toInt();
        
        if (QMessageBox::question(this, tr("Confirm Delete"),
                tr("Remove this subject room requirement?")) == QMessageBox::Yes) {
            dm->subjectRequirements.erase(std::remove_if(dm->subjectRequirements.begin(), dm->subjectRequirements.end(),
                [subjectId, roomTypeId](const SubjectRequirement &sr) {
                    return sr.subjectId == subjectId && sr.roomTypeId == roomTypeId;
                }), dm->subjectRequirements.end());
            setupSubjectRequirementsTab();
        }
    }
}

void RoomTypeWidget::refresh() {
    setupRoomTypesTab();
    setupSubjectRequirementsTab();
}

void RoomTypeWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}
