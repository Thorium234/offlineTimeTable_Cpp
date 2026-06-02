#pragma once
#include <string>
#include <vector>
#include <map>

const int DAYS = 5;
const int PERIODS = 8;

extern const std::string DAY_NAMES[DAYS];

class DataManager; // Forward declaration

class Timetable {
public:
    // Maps classId -> 5x8 grid of subject/lesson string representation
    std::map<int, std::vector<std::vector<std::string>>> schedules;

    void initClass(int classId);
    void setSlot(int classId, int day, int period, const std::string& subjectName);
    std::string getSlot(int classId, int day, int period) const;
    void print(const DataManager& dm) const;
};
