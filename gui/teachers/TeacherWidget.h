#pragma once
#include <QShowEvent>

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QStandardItemModel>
#include "../../services/DataManager.h"

class TeacherWidget : public QWidget {
    Q_OBJECT
public:
    explicit TeacherWidget(DataManager *dm, QWidget *parent = nullptr);
    ~TeacherWidget() override = default;

protected:
    void showEvent(QShowEvent *event) override;

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
    void addTeacher();
    void editTeacher();
    void deleteTeacher();
};
