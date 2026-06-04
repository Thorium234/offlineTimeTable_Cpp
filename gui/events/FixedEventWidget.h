#pragma once

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include "../services/DataManager.h"

class FixedEventWidget : public QWidget {
    Q_OBJECT
public:
    explicit FixedEventWidget(DataManager *dm, QWidget *parent = nullptr);
    void refresh();

private:
    void setupUi();
    void setupModelAndView();
    void connectSignals();
    void addFixedEvent();
    void deleteFixedEvent();

    DataManager *dm;
    QTableView *tableView;
    QStandardItemModel *model;
    QComboBox *dayCombo;
    QComboBox *periodCombo;
    QComboBox *recurrenceCombo;
    QLineEdit *nameEdit;
    QPushButton *addBtn;
    QPushButton *delBtn;
};
