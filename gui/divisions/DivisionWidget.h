#pragma once

#include <QWidget>
#include <QShowEvent>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QStandardItemModel>
#include "../../services/DataManager.h"

class DivisionWidget : public QWidget {
    Q_OBJECT
public:
    explicit DivisionWidget(DataManager *dm, QWidget *parent = nullptr);
    void refresh();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void setupModelAndView();
    void connectSignals();
    void addDivision();
    void deleteDivision();
    void assignClass();

    DataManager *dm;
    QTableView *tableView;
    QStandardItemModel *model;
    QLineEdit *nameEdit;
    QCheckBox *parallelCheck;
    QComboBox *classCombo;
    QComboBox *divisionCombo;
    QPushButton *addBtn;
    QPushButton *delBtn;
    QPushButton *assignBtn;
};
