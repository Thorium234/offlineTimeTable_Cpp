#include "TimeSlotWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QHeaderView>

TimeSlotWidget::TimeSlotWidget(DataManager *dm, QWidget *parent)
    : QWidget(parent), dm(dm) {
    setWindowTitle(tr("Days & Periods Management"));
    resize(800, 500);
    setupUi();
    connectSignals();
}

void TimeSlotWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QTabWidget *tabWidget = new QTabWidget(this);
    
    // Days tab
    QWidget *daysTab = new QWidget(this);
    QVBoxLayout *daysLayout = new QVBoxLayout(daysTab);
    
    daysTableView = new QTableView(daysTab);
    daysTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    daysTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    daysTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    daysLayout->addWidget(daysTableView);
    
    QHBoxLayout *daysBtnLayout = new QHBoxLayout;
    dayNameEdit = new QLineEdit(daysTab);
    dayNameEdit->setPlaceholderText(tr("Day Name (e.g., Monday)"));
    addDayBtn = new QPushButton(tr("Add Day"), daysTab);
    delDayBtn = new QPushButton(tr("Delete"), daysTab);
    daysBtnLayout->addWidget(dayNameEdit);
    daysBtnLayout->addWidget(addDayBtn);
    daysBtnLayout->addWidget(delDayBtn);
    daysLayout->addLayout(daysBtnLayout);
    
    // Periods tab
    QWidget *periodsTab = new QWidget(this);
    QVBoxLayout *periodsLayout = new QVBoxLayout(periodsTab);
    
    periodsTableView = new QTableView(periodsTab);
    periodsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    periodsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    periodsTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    periodsLayout->addWidget(periodsTableView);
    
    QHBoxLayout *periodsBtnLayout = new QHBoxLayout;
    startTimeEdit = new QLineEdit(periodsTab);
    startTimeEdit->setPlaceholderText(tr("Start Time (e.g., 08:00)"));
    endTimeEdit = new QLineEdit(periodsTab);
    endTimeEdit->setPlaceholderText(tr("End Time (e.g., 08:45)"));
    addPeriodBtn = new QPushButton(tr("Add Period"), periodsTab);
    delPeriodBtn = new QPushButton(tr("Delete"), periodsTab);
    periodsBtnLayout->addWidget(startTimeEdit);
    periodsBtnLayout->addWidget(endTimeEdit);
    periodsBtnLayout->addWidget(addPeriodBtn);
    periodsBtnLayout->addWidget(delPeriodBtn);
    periodsLayout->addLayout(periodsBtnLayout);
    
    tabWidget->addTab(daysTab, tr("Days"));
    tabWidget->addTab(periodsTab, tr("Periods"));
    
    mainLayout->addWidget(tabWidget);
    
    setupDaysTab();
    setupPeriodsTab();
}

void TimeSlotWidget::setupDaysTab() {
    daysModel = new QStandardItemModel(this);
    daysModel->setColumnCount(2);
    daysModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    daysModel->setHeaderData(1, Qt::Horizontal, tr("Name"));
    
    for (const auto &d : dm->days) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::number(d.id));
        rowItems << new QStandardItem(QString::fromStdString(d.name));
        daysModel->appendRow(rowItems);
    }
    daysTableView->setModel(daysModel);
    daysTableView->hideColumn(0);
    daysTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void TimeSlotWidget::setupPeriodsTab() {
    periodsModel = new QStandardItemModel(this);
    periodsModel->setColumnCount(3);
    periodsModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
    periodsModel->setHeaderData(1, Qt::Horizontal, tr("Start Time"));
    periodsModel->setHeaderData(2, Qt::Horizontal, tr("End Time"));
    
    for (const auto &p : dm->periods) {
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(QString::number(p.id));
        rowItems << new QStandardItem(QString::fromStdString(p.startTime));
        rowItems << new QStandardItem(QString::fromStdString(p.endTime));
        periodsModel->appendRow(rowItems);
    }
    periodsTableView->setModel(periodsModel);
    periodsTableView->hideColumn(0);
    periodsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void TimeSlotWidget::connectSignals() {
    connect(addDayBtn, &QPushButton::clicked, this, &TimeSlotWidget::addDay);
    connect(delDayBtn, &QPushButton::clicked, this, &TimeSlotWidget::deleteDay);
    connect(addPeriodBtn, &QPushButton::clicked, this, &TimeSlotWidget::addPeriod);
    connect(delPeriodBtn, &QPushButton::clicked, this, &TimeSlotWidget::deletePeriod);
}

void TimeSlotWidget::addDay() {
    QString name = dayNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Please enter a day name."));
        return;
    }
    dm->addDay(name.toStdString());
    dayNameEdit->clear();
    setupDaysTab();
}

void TimeSlotWidget::deleteDay() {
    QModelIndex cur = daysTableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select Day"), tr("Please select a day to delete."));
        return;
    }
    
    int row = cur.row();
    int id = daysModel->item(row, 0)->text().toInt();
    
    if (QMessageBox::question(this, tr("Confirm Delete"),
            tr("Delete day with ID %1?").arg(id)) == QMessageBox::Yes) {
        // Remove from vector
        dm->days.erase(std::remove_if(dm->days.begin(), dm->days.end(),
            [id](const Day &d) { return d.id == id; }), dm->days.end());
        setupDaysTab();
    }
}

void TimeSlotWidget::addPeriod() {
    QString startTime = startTimeEdit->text().trimmed();
    QString endTime = endTimeEdit->text().trimmed();
    
    if (startTime.isEmpty() || endTime.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Please enter both start and end times."));
        return;
    }
    
    dm->addPeriod(startTime.toStdString(), endTime.toStdString());
    startTimeEdit->clear();
    endTimeEdit->clear();
    setupPeriodsTab();
}

void TimeSlotWidget::deletePeriod() {
    QModelIndex cur = periodsTableView->currentIndex();
    if (!cur.isValid()) {
        QMessageBox::information(this, tr("Select Period"), tr("Please select a period to delete."));
        return;
    }
    
    int row = cur.row();
    int id = periodsModel->item(row, 0)->text().toInt();
    
    if (QMessageBox::question(this, tr("Confirm Delete"),
            tr("Delete period with ID %1?").arg(id)) == QMessageBox::Yes) {
        dm->periods.erase(std::remove_if(dm->periods.begin(), dm->periods.end(),
            [id](const Period &p) { return p.id == id; }), dm->periods.end());
        setupPeriodsTab();
    }
}

void TimeSlotWidget::refresh() {
    setupDaysTab();
    setupPeriodsTab();
}

void TimeSlotWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}
