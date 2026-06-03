#pragma once
#include <QShowEvent>

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QStandardItemModel>
#include "../../services/DataManager.h"

class RoomWidget : public QWidget {
    Q_OBJECT
public:
    explicit RoomWidget(DataManager *dm, QWidget *parent = nullptr);
    ~RoomWidget() override = default;

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
    void addRoom();
    void editRoom();
    void deleteRoom();
};
