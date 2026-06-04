#pragma once

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QLineEdit>
#include <QTabWidget>
#include "../services/DataManager.h"

class TimeSlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimeSlotWidget(DataManager *dm, QWidget *parent = nullptr);
    void refresh();

private:
    void setupUi();
    void setupDaysTab();
    void setupPeriodsTab();
    void connectSignals();
    void addDay();
    void addPeriod();
    void deleteDay();
    void deletePeriod();

    DataManager *dm;
    
    // Days tab
    QTableView *daysTableView;
    QStandardItemModel *daysModel;
    QLineEdit *dayNameEdit;
    QPushButton *addDayBtn;
    QPushButton *delDayBtn;
    
    // Periods tab
    QTableView *periodsTableView;
    QStandardItemModel *periodsModel;
    QLineEdit *startTimeEdit;
    QLineEdit *endTimeEdit;
    QPushButton *addPeriodBtn;
    QPushButton *delPeriodBtn;
};
