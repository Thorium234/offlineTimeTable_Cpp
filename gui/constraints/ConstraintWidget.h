#pragma once

#include <QWidget>
#include <QShowEvent>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QTabWidget>
#include <QStandardItemModel>
#include "../../services/DataManager.h"

class ConstraintWidget : public QWidget {
    Q_OBJECT
public:
    explicit ConstraintWidget(DataManager *dm, QWidget *parent = nullptr);
    void refresh();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void setupTeacherConstraintsTab();
    void connectSignals();
    void addTeacherConstraint();
    void deleteTeacherConstraint();

    DataManager *dm;
    
    // Teacher Constraints
    QTableView *constraintsTableView;
    QStandardItemModel *constraintsModel;
    QComboBox *teacherCombo;
    QComboBox *dayCombo;
    QComboBox *periodCombo;
    QPushButton *addConstraintBtn;
    QPushButton *delConstraintBtn;
};
