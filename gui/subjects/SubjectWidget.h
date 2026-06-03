#pragma once

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QStandardItemModel>
#include "../../services/DataManager.h"

class SubjectWidget : public QWidget {
    Q_OBJECT
public:
    explicit SubjectWidget(DataManager *dm, QWidget *parent = nullptr);
    ~SubjectWidget() override = default;

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
    void addSubject();
    void editSubject();
    void deleteSubject();
};
