#include "LessonWidget.h"
#include "LessonDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QHeaderView>

LessonWidget::LessonWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Lesson Management"));
    resize(900, 500);
    setupUi();
    setupModelAndView();
    connectSignals();
}

void LessonWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    tableView = new QTableView(this);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(tableView);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    addBtn = new QPushButton(tr("Add Lesson"), this);
    delBtn = new QPushButton(tr("Delete"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    mainLayout->addLayout(btnLayout);
}

void LessonWidget::setupModelAndView() {
    model = new QStandardItemModel(this);
    model->setColumnCount(10);
    model->setHeaderData(0, Qt::Horizontal, tr("Teacher"));
    model->setHeaderData(1, Qt::Horizontal, tr("2nd Teacher"));
    model->setHeaderData(2, Qt::Horizontal, tr("Subject"));
    model->setHeaderData(3, Qt::Horizontal, tr("Class"));
    model->setHeaderData(4, Qt::Horizontal, tr("Combined"));
    model->setHeaderData(5, Qt::Horizontal, tr("Periods/Week"));
    model->setHeaderData(6, Qt::Horizontal, tr("Block Size"));
    model->setHeaderData(7, Qt::Horizontal, tr("Max/Day"));
    model->setHeaderData(8, Qt::Horizontal, tr("Week"));
    model->setHeaderData(9, Qt::Horizontal, tr("ID"));

    int idx = 0;
    for (const auto &l : dm->lessons) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::fromStdString(dm->getTeacherName(l.teacherId)));
        QString secondTeacher = l.secondTeacherId >= 0
            ? QString::fromStdString(dm->getTeacherName(l.secondTeacherId)) : QStringLiteral("—");
        rowItems << new QStandardItem(secondTeacher);
        rowItems << new QStandardItem(QString::fromStdString(dm->getSubjectName(l.subjectId)));
        rowItems << new QStandardItem(QString::fromStdString(dm->getClassName(l.classId)));
        QString combined;
        for (size_t ci = 0; ci < l.combinedClassIds.size(); ++ci) {
            if (ci > 0) combined += QStringLiteral(", ");
            combined += QString::fromStdString(dm->getClassName(l.combinedClassIds[ci]));
        }
        if (combined.isEmpty()) combined = QStringLiteral("—");
        rowItems << new QStandardItem(combined);
        rowItems << new QStandardItem(QString::number(l.periodsPerWeek));
        rowItems << new QStandardItem(QString::number(l.blockSize));
        rowItems << new QStandardItem(l.maxPerDay == 0 ? "N/A" : QString::number(l.maxPerDay));
        QString weekLabel;
        if (l.weekType == 0) weekLabel = tr("Every");
        else if (l.weekType == 1) weekLabel = tr("Week A");
        else weekLabel = tr("Week B");
        rowItems << new QStandardItem(weekLabel);
        rowItems << new QStandardItem(QString::number(idx)); // Store index
        model->appendRow(rowItems);
        idx++;
    }
    tableView->setModel(model);
    tableView->hideColumn(9); // Hide index column
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void LessonWidget::connectSignals() {
    connect(addBtn, &QPushButton::clicked, this, &LessonWidget::addLesson);
    connect(delBtn, &QPushButton::clicked, this, &LessonWidget::deleteLesson);
}

void LessonWidget::addLesson() {
    if (dm->teachers.empty() || dm->subjects.empty() || dm->classes.empty()) {
        QMessageBox::warning(this, tr("Cannot Add"),
            tr("Please add at least one teacher, subject, and class first."));
        return;
    }
    
    LessonDialog dlg(this);
    dlg.loadFromDataManager(dm);
    
    if (dlg.exec() == QDialog::Accepted) {
        dm->addLesson(
            dlg.teacherId(),
            dlg.subjectId(),
            dlg.classId(),
            dlg.periodsPerWeek(),
            dlg.blockSize(),
            dlg.maxPerDay(),
            dlg.weekType(),
            dlg.secondTeacherId(),
            dlg.combinedClassIds()
        );
        refresh();
    }
}

void LessonWidget::deleteLesson() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select Lesson"),
            tr("Please select a lesson to delete."));
        return;
    }
    
    int row = cur.row();
    int idx = model->item(row, 8)->text().toInt();
    
    if (QMessageBox::question(this, tr("Confirm Delete"),
            tr("Delete this lesson?")) == QMessageBox::Yes) {
        if (idx >= 0 && idx < static_cast<int>(dm->lessons.size())) {
            dm->removeLesson(dm->lessons[idx].id);
            refresh();
        }
    }
}

void LessonWidget::refresh() {
    setupModelAndView();
}

void LessonWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}
