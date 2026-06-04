#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>
#include "../services/DataManager.h"
#include "../../timetable/Timetable.h"

class TimetableViewWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimetableViewWidget(DataManager *dm, QWidget *parent = nullptr);
    void setTimetable(const Timetable &t);
    void refresh();

private:
    void setupUi();
    void setupClassTimetableTab();
    void setupTeacherTimetableTab();
    void setupRoomTimetableTab();
    void populateClassTimetable(const QString &className);
    void populateTeacherTimetable(const QString &teacherName);
    void populateRoomTimetable(const QString &roomName);

    DataManager *dm;
    Timetable timetable;
    
    // Class timetable
    QComboBox *classCombo;
    QTableWidget *classTableWidget;
    
    // Teacher timetable
    QComboBox *teacherCombo;
    QTableWidget *teacherTableWidget;
    
    // Room timetable
    QComboBox *roomCombo;
    QTableWidget *roomTableWidget;
};
