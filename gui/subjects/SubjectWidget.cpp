#include "SubjectWidget.h"
#include "SubjectDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStandardItem>

SubjectWidget::SubjectWidget(DataManager *dm, QWidget *parent) : QWidget(parent), dm(dm) {
    resize(800, 500);
    setWindowTitle(tr("Subject Management"));
    setupUi();
    setupModelAndView();
    connectSignals();
}

void SubjectWidget::setupUi() {
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

void SubjectWidget::setupModelAndView() {
    model = new QStandardItemModel(this);
    model->setColumnCount(2);
    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Name"));

    const auto &subjects = dm->subjects;
    for (const auto &s : subjects) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::number(s.id));
        rowItems << new QStandardItem(QString::fromStdString(s.name));
        model->appendRow(rowItems);
    }
    tableView->setModel(model);
    tableView->hideColumn(0);
}

void SubjectWidget::connectSignals() {
    connect(addBtn, &QPushButton::clicked, this, &SubjectWidget::addSubject);
    connect(editBtn, &QPushButton::clicked, this, &SubjectWidget::editSubject);
    connect(delBtn, &QPushButton::clicked, this, &SubjectWidget::deleteSubject);
}

void SubjectWidget::addSubject() {
    SubjectDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString name = dlg.subjectName();
        if (name.isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Input"), tr("Subject name cannot be empty."));
            return;
        }
        int newId = dm->addSubject(name.toStdString());
        if (newId > 0) {
            QList<QStandardItem*> rowItems;
            rowItems << new QStandardItem(QString::number(newId));
            rowItems << new QStandardItem(name);
            model->appendRow(rowItems);
        } else {
            QMessageBox::warning(this, tr("Add failed"), tr("Could not insert the subject."));
        }
    }
}

void SubjectWidget::editSubject() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select subject"), tr("Please select a subject to edit."));
        return;
    }
    int row = cur.row();
    int id = model->item(row, 0)->text().toInt();
    QString oldName = model->item(row, 1)->text();
    SubjectDialog dlg(this);
    dlg.setSubjectName(oldName);
    if (dlg.exec() == QDialog::Accepted) {
        const QString newName = dlg.subjectName();
        if (newName.isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Input"), tr("Subject name cannot be empty."));
            return;
        }
        if (dm->updateSubject(id, newName.toStdString())) {
            model->item(row, 1)->setText(newName);
        } else {
            QMessageBox::warning(this, tr("Update failed"), tr("Could not update the subject."));
        }
    }
}

void SubjectWidget::deleteSubject() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select subject"), tr("Please select a subject to delete."));
        return;
    }
    int row = cur.row();
    int id = model->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, tr("Confirm delete"),
            tr("Delete subject with ID %1?").arg(id)) == QMessageBox::Yes) {
        if (dm->removeSubject(id)) {
            model->removeRow(row);
        } else {
            QMessageBox::warning(this, tr("Delete failed"), tr("Could not delete the subject."));
        }
    }
}

void SubjectWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    setupModelAndView(); // Refresh data when tab is shown
}
