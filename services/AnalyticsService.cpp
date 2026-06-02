#include "AnalyticsService.h"
#include "../timetable/Timetable.h"
#include <unordered_map>

AnalyticsReport AnalyticsService::generateReport(const Timetable& timetable) {
    AnalyticsReport report;
    int occupied = 0;
    int totalSlots = 0;
    std::unordered_map<int, int> teacherLessonCount;

    // Determine total days and periods from any class schedule (assume uniform)
    int days = 0, periods = 0;
    if (!timetable.schedules.empty()) {
        const auto& firstSchedule = timetable.schedules.begin()->second;
        days = static_cast<int>(firstSchedule.size());
        if (days > 0) periods = static_cast<int>(firstSchedule[0].size());
    }
    totalSlots = static_cast<int>(timetable.schedules.size()) * days * periods;

    for (const auto& classEntry : timetable.schedules) {
        for (const auto& dayVec : classEntry.second) {
            for (const auto& cell : dayVec) {
                if (!cell.isEmpty()) {
                    ++occupied;
                    ++teacherLessonCount[cell.teacherId];
                }
            }
        }
    }

    report.totalLessons = occupied;
    report.unscheduledLessons = static_cast<int>(timetable.unscheduledLessons.size());
    report.averageRoomUtilization = totalSlots ? (static_cast<double>(occupied) / totalSlots) * 100.0 : 0.0;
    // Average teacher load: total lessons / number of teachers (if any)
    report.averageTeacherLoad = teacherLessonCount.empty() ? 0.0 : static_cast<double>(occupied) / teacherLessonCount.size();
    report.conflictCount = report.unscheduledLessons; // simple conflict metric

    // Populate detailed teacher loads and room utilizations
    report.teacherLoads = computeTeacherLoads(timetable);
    report.roomUtilizations = computeRoomUtilizations(timetable);

    // No detailed notes for now
    return report;
}

double AnalyticsService::computeRoomUtilization(const Timetable& tt) {
    // Deprecated: logic moved to generateReport
    return 0.0;
}

double AnalyticsService::computeTeacherLoad(const Timetable& tt) {
    return 0.0;
}

int AnalyticsService::countConflicts(const Timetable& tt) {
    return static_cast<int>(tt.unscheduledLessons.size());
}

int AnalyticsService::countUnscheduled(const Timetable& tt) {
    return static_cast<int>(tt.unscheduledLessons.size());
}
