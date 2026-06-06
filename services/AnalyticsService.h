#pragma once

#include <vector>
#include <string>
#include "../timetable/Timetable.h"
#include "DataManager.h"

struct TeacherLoad {
    int teacherId;
    std::string name;
    int assignedPeriods = 0;
    int availablePeriods = 0;
    double utilization = 0.0; // percentage
};

struct RoomUtilizationInfo {
    int roomId;
    std::string name;
    int usedSlots = 0;
    int totalSlots = 0;
    double utilization = 0.0; // percentage
};

struct ClassGapInfo {
    int classId;
    std::string className;
    int totalGaps = 0;
    int maxGap = 0;
    int gapPeriods = 0;
};

struct SubjectDistributionInfo {
    int classId;
    std::string className;
    std::string subjectName;
    int slotCount = 0;
};

struct WeekTypeDistInfo {
    int weekType = 0;
    int count = 0;
};

struct AnalyticsReport {
    int totalLessons = 0;
    int unscheduledLessons = 0;
    double averageRoomUtilization = 0.0; // percentage
    double averageTeacherLoad = 0.0;     // periods per teacher
    int conflictCount = 0;
    std::vector<std::string> notes; // optional explanations
    std::vector<TeacherLoad> teacherLoads;
    std::vector<RoomUtilizationInfo> roomUtilizations;
    std::vector<ClassGapInfo> classGaps;
    std::vector<SubjectDistributionInfo> subjectDistribution;
    std::vector<WeekTypeDistInfo> weekTypeDistribution;
};

class AnalyticsService {
public:
    AnalyticsService() = default;
    AnalyticsReport generateReport(const Timetable& timetable, const DataManager& dm);

private:
    int countConflicts(const Timetable& tt);
    int countUnscheduled(const Timetable& tt);
    std::vector<TeacherLoad>          computeTeacherLoads(const Timetable& tt, const DataManager& dm);
    std::vector<RoomUtilizationInfo>  computeRoomUtilizations(const Timetable& tt, const DataManager& dm);
};
