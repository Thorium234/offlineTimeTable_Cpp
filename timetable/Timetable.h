#pragma once
#include <string>
#include <vector>
#include <map>

struct TimetableCell {
    int subjectId = -1;
    int teacherId = -1;
    int roomId = -1;
    bool locked = false;
    int weekType = 0; // 0=every week, 1=week A only, 2=week B only

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
    void setSlot(int classId, int dayIndex, int periodIndex, int subjectId, int teacherId, int roomId, int weekType = 0);
    void setSlotLocked(int classId, int dayIndex, int periodIndex, int subjectId, int teacherId, int roomId, bool locked = true);
    void clearSlot(int classId, int dayIndex, int periodIndex);
    bool moveSlot(int srcClassId, int srcDay, int srcPeriod,
                  int dstClassId, int dstDay, int dstPeriod);
    bool swapSlots(int classA, int dayA, int periodA,
                   int classB, int dayB, int periodB);
    TimetableCell getSlot(int classId, int dayIndex, int periodIndex) const;
    bool isSlotLocked(int classId, int dayIndex, int periodIndex) const;
    void print(const DataManager& dm) const;
};
