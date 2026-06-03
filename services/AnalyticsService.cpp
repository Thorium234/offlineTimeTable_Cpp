#include "AnalyticsService.h"
#include "../timetable/Timetable.h"
#include <unordered_map>

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

AnalyticsReport AnalyticsService::generateReport(const Timetable& timetable, const DataManager& dm) {
    AnalyticsReport report;

    int numDays    = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());
    int slotsPerWeek = numDays * numPeriods;

    // --- Total occupied lesson slots ---
    int occupied = 0;
    for (const auto& classEntry : timetable.schedules) {
        for (const auto& dayVec : classEntry.second) {
            for (const auto& cell : dayVec) {
                if (!cell.isEmpty()) {
                    ++occupied;
                }
            }
        }
    }
    report.totalLessons        = occupied;
    report.unscheduledLessons  = static_cast<int>(timetable.unscheduledLessons.size());
    report.conflictCount       = report.unscheduledLessons;

    // --- Room utilisation (average across all rooms) ---
    auto roomUtils = computeRoomUtilizations(timetable, dm);
    report.roomUtilizations = roomUtils;
    if (!roomUtils.empty()) {
        double sum = 0.0;
        for (const auto& r : roomUtils) sum += r.utilization;
        report.averageRoomUtilization = sum / roomUtils.size();
    }

    // --- Teacher load (average periods per teacher) ---
    auto teacherLoads = computeTeacherLoads(timetable, dm);
    report.teacherLoads = teacherLoads;
    if (!teacherLoads.empty()) {
        double sum = 0.0;
        for (const auto& t : teacherLoads) sum += t.assignedPeriods;
        report.averageTeacherLoad = sum / teacherLoads.size();
    }

    (void)slotsPerWeek; // suppress unused warning when rooms list is empty
    return report;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int AnalyticsService::countConflicts(const Timetable& tt) {
    return static_cast<int>(tt.unscheduledLessons.size());
}

int AnalyticsService::countUnscheduled(const Timetable& tt) {
    return static_cast<int>(tt.unscheduledLessons.size());
}

std::vector<TeacherLoad> AnalyticsService::computeTeacherLoads(
        const Timetable& tt, const DataManager& dm) {

    int numDays    = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());
    int totalSlots = numDays * numPeriods;

    // Build a map: teacherId -> number of occupied slots in the schedule
    std::unordered_map<int, int> assigned;
    for (const auto& t : dm.teachers) assigned[t.id] = 0;

    for (const auto& classEntry : tt.schedules) {
        for (const auto& dayVec : classEntry.second) {
            for (const auto& cell : dayVec) {
                if (!cell.isEmpty()) {
                    assigned[cell.teacherId]++;
                }
            }
        }
    }

    std::vector<TeacherLoad> loads;
    loads.reserve(dm.teachers.size());

    for (const auto& t : dm.teachers) {
        // Count explicit unavailability constraints for this teacher
        int unavailCount = 0;
        for (const auto& tc : dm.teacherConstraints) {
            if (tc.teacherId == t.id) ++unavailCount;
        }

        TeacherLoad tl;
        tl.teacherId       = t.id;
        tl.name            = t.name;
        tl.assignedPeriods = assigned[t.id];
        tl.availablePeriods = totalSlots - unavailCount;
        tl.utilization     = (tl.availablePeriods > 0)
                             ? (static_cast<double>(tl.assignedPeriods) / tl.availablePeriods) * 100.0
                             : 0.0;
        loads.push_back(tl);
    }
    return loads;
}

std::vector<RoomUtilizationInfo> AnalyticsService::computeRoomUtilizations(
        const Timetable& tt, const DataManager& dm) {

    int numDays    = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());
    int totalSlots = numDays * numPeriods; // maximum slots any room can be occupied

    // Build a map: roomId -> occupied slot count
    std::unordered_map<int, int> usedMap;
    for (const auto& r : dm.rooms) usedMap[r.id] = 0;

    for (const auto& classEntry : tt.schedules) {
        for (const auto& dayVec : classEntry.second) {
            for (const auto& cell : dayVec) {
                if (!cell.isEmpty() && cell.roomId != -1) {
                    usedMap[cell.roomId]++;
                }
            }
        }
    }

    std::vector<RoomUtilizationInfo> utils;
    utils.reserve(dm.rooms.size());

    for (const auto& r : dm.rooms) {
        RoomUtilizationInfo ri;
        ri.roomId     = r.id;
        ri.name       = r.name;
        ri.usedSlots  = usedMap[r.id];
        ri.totalSlots = totalSlots;
        ri.utilization = (totalSlots > 0)
                         ? (static_cast<double>(ri.usedSlots) / totalSlots) * 100.0
                         : 0.0;
        utils.push_back(ri);
    }
    return utils;
}
