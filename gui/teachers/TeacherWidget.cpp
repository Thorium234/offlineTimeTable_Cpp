#include "TeacherWidget.h"
#include "TeacherDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QTableView>
#include <QPushButton>

TeacherWidget::TeacherWidget(DataManager *dm, QWidget *parent) : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Teacher Management"));
    resize(800, 500);
    setupUi();
    setupModelAndView();
    connectSignals();
}

void TeacherWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    tableView = new QTableView(this);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(tableView);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    addBtn = new QPushButton(tr("Add"), this);
    editBtn = new QPushButton(tr("Edit"), this);
    delBtn = new QPushButton(tr("Delete"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(editBtn);
    btnLayout->addWidget(delBtn);
    mainLayout->addLayout(btnLayout);
}

void TeacherWidget::setupModelAndView() {
    // Populate model from DataManager vectors
    model = new QStandardItemModel(this);
    model->setColumnCount(2);
    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Name"));
    // Fill rows
    const auto &teachers = dm->teachers;
    for (const auto &t : teachers) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::number(t.id));
        rowItems << new QStandardItem(QString::fromStdString(t.name));
        model->appendRow(rowItems);
    }
    tableView->setModel(model);
    tableView->hideColumn(0); // hide ID column
}

void TeacherWidget::connectSignals() {
    connect(addBtn, &QPushButton::clicked, this, &TeacherWidget::addTeacher);
    connect(editBtn, &QPushButton::clicked, this, &TeacherWidget::editTeacher);
    connect(delBtn, &QPushButton::clicked, this, &TeacherWidget::deleteTeacher);
}

void TeacherWidget::addTeacher() {
    TeacherDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString name = dlg.teacherName();
        int newId = dm->addTeacher(name.toStdString());
        if (newId > 0) {
            // Append to model
            QList<QStandardItem*> rowItems;
            rowItems << new QStandardItem(QString::number(newId));
            rowItems << new QStandardItem(name);
            model->appendRow(rowItems);
        } else {
            QMessageBox::warning(this, tr("Add failed"), tr("Could not insert the teacher."));
        }
    }
}

void TeacherWidget::editTeacher() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select teacher"), tr("Please select a teacher to edit."));
        return;
    }
    int row = cur.row();
    int id = model->item(row, 0)->text().toInt();
    QString oldName = model->item(row, 1)->text();
    TeacherDialog dlg(this);
    dlg.setTeacherName(oldName);
    if (dlg.exec() == QDialog::Accepted) {
        const QString newName = dlg.teacherName();
        if (dm->updateTeacher(id, newName.toStdString())) {
            model->item(row, 1)->setText(newName);
        } else {
            QMessageBox::warning(this, tr("Update failed"), tr("Could not update the teacher."));
        }
    }
}

void TeacherWidget::deleteTeacher() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select teacher"), tr("Please select a teacher to delete."));
        return;
    }
    int row = cur.row();
    int id = model->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, tr("Confirm delete"),
            tr("Delete teacher with ID %1?").arg(id)) == QMessageBox::Yes) {
        if (dm->removeTeacher(id)) {
            model->removeRow(row);
        } else {
            QMessageBox::warning(this, tr("Delete failed"), tr("Could not delete the teacher."));
        }
    }
}

void TeacherWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    setupModelAndView(); // Refresh data when tab is shown
}
