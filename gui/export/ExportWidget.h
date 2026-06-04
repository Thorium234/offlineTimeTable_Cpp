#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include "../services/DataManager.h"
#include "../../timetable/Timetable.h"

class ExportWidget : public QWidget {
    Q_OBJECT
public:
    explicit ExportWidget(DataManager *dm, QWidget *parent = nullptr);
    void setTimetable(const Timetable &t);
    void refresh();

private:
    void setupUi();
    void exportToCSV();
    void exportToHTML();

    DataManager *dm;
    Timetable timetable;
    
    QLineEdit *filenameEdit;
    QComboBox *formatCombo;
    QPushButton *exportBtn;
    QLabel *statusLabel;
};
