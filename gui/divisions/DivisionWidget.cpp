#include "DivisionWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>
#include <QHeaderView>

DivisionWidget::DivisionWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Divisions / Groups"));
    resize(800, 500);
    setupUi();
    setupModelAndView();
    connectSignals();
}

void DivisionWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    tableView = new QTableView(this);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(tableView);

    QGroupBox *addGroup = new QGroupBox(tr("Add Division"), this);
    QHBoxLayout *addLayout = new QHBoxLayout(addGroup);
    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText(tr("Division name..."));
    parallelCheck = new QCheckBox(tr("Classes can run in parallel"), this);
    parallelCheck->setChecked(true);
    addBtn = new QPushButton(tr("Add"), this);
    addLayout->addWidget(new QLabel(tr("Name:"), this));
    addLayout->addWidget(nameEdit, 1);
    addLayout->addWidget(parallelCheck);
    addLayout->addWidget(addBtn);
    mainLayout->addWidget(addGroup);

    QGroupBox *assignGroup = new QGroupBox(tr("Assign Class to Division"), this);
    QHBoxLayout *assignLayout = new QHBoxLayout(assignGroup);
    classCombo = new QComboBox(this);
    divisionCombo = new QComboBox(this);
    assignBtn = new QPushButton(tr("Assign"), this);
    assignLayout->addWidget(new QLabel(tr("Class:"), this));
    assignLayout->addWidget(classCombo, 1);
    assignLayout->addWidget(new QLabel(tr("Division:"), this));
    assignLayout->addWidget(divisionCombo, 1);
    assignLayout->addWidget(assignBtn);
    mainLayout->addWidget(assignGroup);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    delBtn = new QPushButton(tr("Delete Division"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(delBtn);
    mainLayout->addLayout(btnLayout);
}

void DivisionWidget::setupModelAndView() {
    classCombo->clear();
    for (const auto &c : dm->classes) {
        classCombo->addItem(QString::fromStdString(c.name), c.id);
    }

    divisionCombo->clear();
    for (const auto &d : dm->divisions) {
        divisionCombo->addItem(QString::fromStdString(d.name), d.id);
    }

    model = new QStandardItemModel(this);
    model->setColumnCount(5);
    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Name"));
    model->setHeaderData(2, Qt::Horizontal, tr("Parallel"));
    model->setHeaderData(3, Qt::Horizontal, tr("Class Count"));
    model->setHeaderData(4, Qt::Horizontal, tr("Classes"));

    for (const auto &d : dm->divisions) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::number(d.id));
        rowItems << new QStandardItem(QString::fromStdString(d.name));
        rowItems << new QStandardItem(d.canRunInParallel ? tr("Yes") : tr("No"));
        rowItems << new QStandardItem(QString::number(d.classIds.size()));
        std::string classNames;
        for (size_t i = 0; i < d.classIds.size(); ++i) {
            if (i > 0) classNames += ", ";
            classNames += dm->getClassName(d.classIds[i]);
        }
        rowItems << new QStandardItem(QString::fromStdString(classNames));
        model->appendRow(rowItems);
    }
    tableView->setModel(model);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->hideColumn(0);
}

void DivisionWidget::connectSignals() {
    connect(addBtn, &QPushButton::clicked, this, &DivisionWidget::addDivision);
    connect(delBtn, &QPushButton::clicked, this, &DivisionWidget::deleteDivision);
    connect(assignBtn, &QPushButton::clicked, this, &DivisionWidget::assignClass);
}

void DivisionWidget::addDivision() {
    QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid"), tr("Enter a division name."));
        return;
    }
    dm->addDivision(name.toStdString(), parallelCheck->isChecked());
    nameEdit->clear();
    setupModelAndView();
}

void DivisionWidget::deleteDivision() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select"), tr("Select a division to delete."));
        return;
    }
    int row = cur.row();
    if (!model->item(row, 0)) return;
    int id = model->item(row, 0)->text().toInt();
    if (QMessageBox::question(this, tr("Confirm"),
            tr("Delete division and unassign all its classes?")) == QMessageBox::Yes) {
        dm->removeDivision(id);
        setupModelAndView();
    }
}

void DivisionWidget::assignClass() {
    if (classCombo->count() == 0 || divisionCombo->count() == 0) {
        QMessageBox::warning(this, tr("Cannot Assign"),
            tr("Ensure both classes and divisions exist."));
        return;
    }
    int classId = classCombo->currentData().toInt();
    int divisionId = divisionCombo->currentData().toInt();
    dm->assignClassToDivision(classId, divisionId);
    setupModelAndView();
}

void DivisionWidget::refresh() {
    setupModelAndView();
}

void DivisionWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}
