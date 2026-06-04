#pragma once

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include "../services/DataManager.h"

class PreferenceWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreferenceWidget(DataManager *dm, QWidget *parent = nullptr);
    void refresh();

private:
    void setupUi();
    void setupModelAndView();
    void connectSignals();
    void addPreference();
    void deletePreference();

    DataManager *dm;
    QTableView *tableView;
    QStandardItemModel *model;
    QComboBox *teacherCombo;
    QComboBox *dayCombo;
    QComboBox *periodCombo;
    QComboBox *typeCombo;
    QPushButton *addBtn;
    QPushButton *delBtn;
};
