#pragma once

#include <QWidget>
#include <QShowEvent>
#include <QTableView>
#include <QPushButton>
#include <QStandardItemModel>
#include "../../services/DataManager.h"

class LessonWidget : public QWidget {
    Q_OBJECT
public:
    explicit LessonWidget(DataManager *dm, QWidget *parent = nullptr);
    void refresh();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void setupModelAndView();
    void connectSignals();
    void addLesson();
    void deleteLesson();

    DataManager *dm;
    QTableView *tableView;
    QStandardItemModel *model;
    QPushButton *addBtn;
    QPushButton *delBtn;
};
