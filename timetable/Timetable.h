#pragma once
#include <string>
#include <vector>
#include <map>

struct TimetableCell {
    int subjectId = -1;
    int teacherId = -1;
    int roomId = -1;

    bool isEmpty() const {
        return subjectId == -1;
    }
};

struct UnscheduledLesson {
    int subjectId;
    int classId;
    int teacherId;
    int periodsCount;
    std::string reason;
};

class DataManager; // Forward declaration

class Timetable {
public:
    // Maps classId -> 2D vector of TimetableCell (dimensions: days x periods)
    std::map<int, std::vector<std::vector<TimetableCell>>> schedules;
    std::vector<UnscheduledLesson> unscheduledLessons;
    int score = 1000;

    void initClass(int classId, int totalDays, int totalPeriods);
    void setSlot(int classId, int dayIndex, int periodIndex, int subjectId, int teacherId, int roomId);
    void clearSlot(int classId, int dayIndex, int periodIndex);
    TimetableCell getSlot(int classId, int dayIndex, int periodIndex) const;
    void print(const DataManager& dm) const;
};
