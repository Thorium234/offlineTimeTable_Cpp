#pragma once
#include "../services/DataManager.h"
#include "../services/TimetableEngine.h"
#include "../timetable/Timetable.h"

class Menu {
private:
    DataManager dm;
    TimetableEngine engine;
    Timetable timetable;
    bool timetableGenerated = false;

    void handleAddTeacher();
    void handleAddSubject();
    void handleAddClass();
    void handleAddRoom();
    void handleAddLesson();
    void handleGenerateTimetable();
    void handleViewTimetable();

    int readIntInput(const std::string& prompt);
    std::string readLineInput(const std::string& prompt);

public:
    void run();
};
