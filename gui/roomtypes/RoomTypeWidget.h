#pragma once

#include <QWidget>
#include <QShowEvent>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QTabWidget>
#include <QStandardItemModel>
#include "../../services/DataManager.h"

class RoomTypeWidget : public QWidget {
    Q_OBJECT
public:
    explicit RoomTypeWidget(DataManager *dm, QWidget *parent = nullptr);
    void refresh();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void setupRoomTypesTab();
    void setupSubjectRequirementsTab();
    void connectSignals();
    void addRoomType();
    void deleteRoomType();
    void addSubjectRequirement();
    void deleteSubjectRequirement();

    DataManager *dm;
    
    // Room Types tab
    QTableView *roomTypesTableView;
    QStandardItemModel *roomTypesModel;
    QLineEdit *roomTypeNameEdit;
    QPushButton *addRoomTypeBtn;
    QPushButton *delRoomTypeBtn;
    
    // Subject Requirements tab
    QTableView *requirementsTableView;
    QStandardItemModel *requirementsModel;
    QComboBox *subjectCombo;
    QComboBox *roomTypeCombo;
    QPushButton *addRequirementBtn;
    QPushButton *delRequirementBtn;
};
