#pragma once

#include <QWidget>
#include <QShowEvent>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QStandardItemModel>
#include "../../services/DataManager.h"

class SubstitutionWidget : public QWidget {
    Q_OBJECT
public:
    explicit SubstitutionWidget(DataManager *dm, QWidget *parent = nullptr);
    void refresh();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void setupModelAndView();
    void connectSignals();
    void addSubstitution();
    void updateSubstitutionStatus(int row);
    void deleteSubstitution();
    void suggestSubstitute();

    DataManager *dm;
    QTableView *tableView;
    QStandardItemModel *model;
    QComboBox *origTeacherCombo;
    QComboBox *subTeacherCombo;
    QComboBox *subjectCombo;
    QComboBox *classCombo;
    QComboBox *dayCombo;
    QComboBox *periodCombo;
    QTextEdit *reasonEdit;
    QPushButton *addBtn;
    QPushButton *delBtn;
    QPushButton *approveBtn;
    QPushButton *completeBtn;
    QPushButton *cancelBtn;
    QPushButton *suggestBtn;
};
