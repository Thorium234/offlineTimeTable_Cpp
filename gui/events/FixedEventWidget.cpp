#include "FixedEventWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QHeaderView>

FixedEventWidget::FixedEventWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Fixed Events"));
    resize(800, 500);
    setupUi();
    setupModelAndView();
    connectSignals();
}

void FixedEventWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Table
    tableView = new QTableView(this);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(tableView);
    
    // Selection controls
    QGridLayout *inputLayout = new QGridLayout;
    
    inputLayout->addWidget(new QLabel(tr("Event Name:"), this), 0, 0);
    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText(tr("e.g., Assembly, Exam"));
    inputLayout->addWidget(nameEdit, 0, 1, 1, 2);
    
    inputLayout->addWidget(new QLabel(tr("Day:"), this), 1, 0);
    dayCombo = new QComboBox(this);
    inputLayout->addWidget(dayCombo, 1, 1);
    
    inputLayout->addWidget(new QLabel(tr("Period:"), this), 1, 2);
    periodCombo = new QComboBox(this);
    inputLayout->addWidget(periodCombo, 1, 3);
    
    inputLayout->addWidget(new QLabel(tr("Recurrence:"), this), 2, 0);
    recurrenceCombo = new QComboBox(this);
    recurrenceCombo->addItem("None", static_cast<int>(RecurrenceType::NONE));
    recurrenceCombo->addItem("Daily", static_cast<int>(RecurrenceType::DAILY));
    recurrenceCombo->addItem("Weekly", static_cast<int>(RecurrenceType::WEEKLY));
    inputLayout->addWidget(recurrenceCombo, 2, 1);
    
    mainLayout->addLayout(inputLayout);
    
    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout;
    addBtn = new QPushButton(tr("Add Fixed Event"), this);
    delBtn = new QPushButton(tr("Delete"), this);
    btnLayout->addStretch();
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    mainLayout->addLayout(btnLayout);
}

void FixedEventWidget::setupModelAndView() {
    // Populate combos
    dayCombo->clear();
    for (const auto &d : dm->days) {
        dayCombo->addItem(QString::fromStdString(d.name), d.id);
    }
    
    periodCombo->clear();
    for (const auto &p : dm->periods) {
        QString periodLabel = QString::fromStdString(p.startTime) + " - " + QString::fromStdString(p.endTime);
        periodCombo->addItem(periodLabel, p.id);
    }
    
    // Setup table
    model = new QStandardItemModel(this);
    model->setColumnCount(5);
    model->setHeaderData(0, Qt::Horizontal, tr("Name"));
    model->setHeaderData(1, Qt::Horizontal, tr("Day"));
    model->setHeaderData(2, Qt::Horizontal, tr("Period"));
    model->setHeaderData(3, Qt::Horizontal, tr("Recurrence"));
    model->setHeaderData(4, Qt::Horizontal, tr("ID"));
    
    for (const auto &e : dm->fixedEvents) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::fromStdString(e.name));
        rowItems << new QStandardItem(QString::fromStdString(dm->getDayName(e.dayId)));
        
        QString periodLabel = QString::fromStdString(dm->getPeriodTime(e.periodId));
        rowItems << new QStandardItem(periodLabel);
        
        rowItems << new QStandardItem(QString::fromStdString(recurrenceToString(e.recurrence)));
        rowItems << new QStandardItem(QString::number(e.id));
        model->appendRow(rowItems);
    }
    tableView->setModel(model);
    tableView->hideColumn(4);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void FixedEventWidget::connectSignals() {
    connect(addBtn, &QPushButton::clicked, this, &FixedEventWidget::addFixedEvent);
    connect(delBtn, &QPushButton::clicked, this, &FixedEventWidget::deleteFixedEvent);
}

void FixedEventWidget::addFixedEvent() {
    QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Please enter an event name."));
        return;
    }
    
    if (dayCombo->count() == 0 || periodCombo->count() == 0) {
        QMessageBox::warning(this, tr("Cannot Add"),
            tr("Please ensure you have at least one day and period defined."));
        return;
    }
    
    int dayId = dayCombo->currentData().toInt();
    int periodId = periodCombo->currentData().toInt();
    RecurrenceType recurrence = static_cast<RecurrenceType>(recurrenceCombo->currentData().toInt());
    
    dm->addFixedEvent(dayId, periodId, name.toStdString(), recurrence);
    nameEdit->clear();
    setupModelAndView();
}

void FixedEventWidget::deleteFixedEvent() {
    QModelIndex cur = tableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select Event"),
            tr("Please select an event to delete."));
        return;
    }
    
    int row = cur.row();
    int id = model->item(row, 4)->text().toInt();
    
    if (QMessageBox::question(this, tr("Confirm Delete"),
            tr("Delete fixed event with ID %1?").arg(id)) == QMessageBox::Yes) {
        dm->removeFixedEvent(id);
        setupModelAndView();
    }
}

void FixedEventWidget::refresh() {
    setupModelAndView();
}

void FixedEventWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}
