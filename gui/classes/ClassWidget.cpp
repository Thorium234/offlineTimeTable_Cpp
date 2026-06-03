#include "ClassWidget.h"
#include "ClassDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStandardItem>

ClassWidget::ClassWidget(DataManager *dm, QWidget *parent) : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Class Management"));
    setupUi();
    setupModelAndView();
    connectSignals();
}

void ClassWidget::setupUi() {
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

void ClassWidget::setupModelAndView() {
    model = new QStandardItemModel(this);
    model->setColumnCount(3);
    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Name"));
    model->setHeaderData(2, Qt::Horizontal, tr("Student Count"));

    const auto &classes = dm->classes;
    for (const auto &c : classes) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::number(c.id));
        rowItems << new QStandardItem(QString::fromStdString(c.name));
        rowItems << new QStandardItem(QString::number(c.studentCount));
        model->appendRow(rowItems);
    }
    tableView->setModel(model);
    tableView->hideColumn(0);
}

void ClassWidget::connectSignals() {
    connect(addBtn, &QPushButton::clicked, this, &ClassWidget::addClass);
    connect(editBtn, &QPushButton::clicked, this, &ClassWidget::editClass);
    connect(delBtn, &QPushButton::clicked, this, &ClassWidget::deleteClass);
}

void ClassWidget::addClass() {
    ClassDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        const QString name = dlg.className();
        int count = dlg.studentCount();
        if (name.isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Input"), tr("Class name cannot be empty."));
            return;
        }
        int newId = dm->addClass(name.toStdString(), count);
        if (newId > 0) {
            QList<QStandardItem*> rowItems;
            rowItems << new QStandardItem(QString::number(newId));
            rowItems << new QStandardItem(name);
            rowItems << new QStandardItem(QString::number(count));
            model->appendRow(rowItems);
        } else {
            QMessageBox::warning(this, tr("Add failed"), tr("Could not insert the class."));
        }
    }
}

void ClassWidget::editClass() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select class"), tr("Please select a class to edit."));
        return;
    }
    int row = cur.row();
    int id = model->item(row, 0)->text().toInt();
    QString oldName = model->item(row, 1)->text();
    int oldCount = model->item(row, 2)->text().toInt();
    ClassDialog dlg(this);
    dlg.setClassName(oldName);
    dlg.setStudentCount(oldCount);
    if (dlg.exec() == QDialog::Accepted) {
        const QString newName = dlg.className();
        int newCount = dlg.studentCount();
        if (newName.isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Input"), tr("Class name cannot be empty."));
            return;
        }
        if (dm->updateClass(id, newName.toStdString(), newCount)) {
            model->item(row, 1)->setText(newName);
            model->item(row, 2)->setText(QString::number(newCount));
        } else {
            QMessageBox::warning(this, tr("Update failed"), tr("Could not update the class."));
        }
    }
}

void ClassWidget::deleteClass() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select class"), tr("Please select a class to delete."));
        return;
    }
    int row = cur.row();
    int id = model->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, tr("Confirm delete"),
            tr("Delete class with ID %1?").arg(id)) == QMessageBox::Yes) {
        if (dm->removeClass(id)) {
            model->removeRow(row);
        } else {
            QMessageBox::warning(this, tr("Delete failed"), tr("Could not delete the class."));
        }
    }
}
