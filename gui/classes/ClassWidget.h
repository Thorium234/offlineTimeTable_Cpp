#pragma once

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QStandardItemModel>
#include "../../services/DataManager.h"

class ClassWidget : public QWidget {
    Q_OBJECT
public:
    explicit ClassWidget(DataManager *dm, QWidget *parent = nullptr);
    ~ClassWidget() override = default;

private:
    void setupUi();
    void setupModelAndView();
    void connectSignals();

    QTableView *tableView;
    QPushButton *addBtn;
    QPushButton *editBtn;
    QPushButton *delBtn;
    QStandardItemModel *model;
    DataManager *dm;

private slots:
    void addClass();
    void editClass();
    void deleteClass();
};
