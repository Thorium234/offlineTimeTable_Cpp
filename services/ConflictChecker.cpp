#include "ConflictChecker.h"
#include "../services/DataManager.h"
#include "../timetable/Timetable.h"

ConflictChecker::ConflictChecker(const DataManager &dataMgr, const Timetable &tbl)
    : dm(dataMgr), tt(tbl) {}

std::vector<ConflictInfo> ConflictChecker::checkPlacement(
    int classId, int subjectId, int teacherId, int roomId,
    int dayIndex, int periodIndex) const {

    std::vector<ConflictInfo> conflicts;

    if (dayIndex < 0 || dayIndex >= static_cast<int>(dm.days.size()) ||
        periodIndex < 0 || periodIndex >= static_cast<int>(dm.periods.size())) {
        return conflicts;
    }

    int dayId = dm.days[dayIndex].id;
    int periodId = dm.periods[periodIndex].id;

    // 1. Self-collision: class already has a lesson at this slot
    auto it = tt.schedules.find(classId);
    if (it != tt.schedules.end()) {
        const auto &grid = it->second;
        if (dayIndex < static_cast<int>(grid.size()) &&
            periodIndex < static_cast<int>(grid[dayIndex].size())) {
            if (!grid[dayIndex][periodIndex].isEmpty()) {
                conflicts.push_back({ConflictInfo::Type::SELF_COLLISION, classId,
                    "Class already has a lesson at this slot", classId, dayIndex, periodIndex});
            }
        }
    }

    // 2. Teacher busy — scan all class schedules
    for (const auto &pair : tt.schedules) {
        int cid = pair.first;
        const auto &grid = pair.second;
        if (dayIndex < static_cast<int>(grid.size()) &&
            periodIndex < static_cast<int>(grid[dayIndex].size())) {
            const auto &cell = grid[dayIndex][periodIndex];
            if (cell.teacherId == teacherId && !cell.isEmpty()) {
                conflicts.push_back({ConflictInfo::Type::TEACHER_BUSY, teacherId,
                    "Teacher is already assigned to another class at this time",
                    cid, dayIndex, periodIndex});
            }
        }
    }

    // 3. Room busy
    for (const auto &pair : tt.schedules) {
        int cid = pair.first;
        const auto &grid = pair.second;
        if (dayIndex < static_cast<int>(grid.size()) &&
            periodIndex < static_cast<int>(grid[dayIndex].size())) {
            const auto &cell = grid[dayIndex][periodIndex];
            if (cell.roomId == roomId && !cell.isEmpty()) {
                conflicts.push_back({ConflictInfo::Type::ROOM_BUSY, roomId,
                    "Room is already occupied by another class at this time",
                    cid, dayIndex, periodIndex});
            }
        }
    }

    // 4. Teacher unavailability constraint
    if (dm.isTeacherUnavailable(teacherId, dayId, periodId)) {
        conflicts.push_back({ConflictInfo::Type::TEACHER_UNAVAILABLE, teacherId,
            "Teacher is unavailable at this time (constraint)"});
    }

    // 5. Fixed events — check if any fixed event blocks teacher/room/class
    for (const auto &fe : dm.fixedEvents) {
        bool dayMatches = false;
        if (fe.recurrence == RecurrenceType::DAILY) {
            dayMatches = true;
        } else if (fe.recurrence == RecurrenceType::WEEKLY || fe.recurrence == RecurrenceType::NONE) {
            dayMatches = (fe.dayId == dayId);
        }
        if (dayMatches && fe.periodId == periodId) {
            conflicts.push_back({ConflictInfo::Type::FIXED_EVENT, fe.id,
                "Fixed event blocks this slot: " + fe.name});
        }
    }

    // 6. Teacher preference undesirable check
    PreferenceType pref = dm.getTeacherPreference(teacherId, dayId, periodId);
    if (pref == PreferenceType::UNDESIRABLE) {
        conflicts.push_back({ConflictInfo::Type::TEACHER_UNAVAILABLE, teacherId,
            "Teacher marks this slot as undesirable"});
    }

    return conflicts;
}

std::vector<ConflictInfo> ConflictChecker::checkPlacementBlock(
    int classId, int subjectId, int teacherId, int roomId,
    int dayIndex, int periodIndex, int blockSize) const {

    std::vector<ConflictInfo> allConflicts;
    int numPeriods = static_cast<int>(dm.periods.size());
    int endPeriod = std::min(periodIndex + blockSize, numPeriods);

    for (int p = periodIndex; p < endPeriod; ++p) {
        auto cellConflicts = checkPlacement(classId, subjectId, teacherId, roomId,
                                            dayIndex, p);
        allConflicts.insert(allConflicts.end(), cellConflicts.begin(),
                            cellConflicts.end());
    }
    return allConflicts;
}

std::vector<ConflictInfo> ConflictChecker::checkMove(
    int srcClassId, int srcDay, int srcPeriod,
    int dstClassId, int dstDay, int dstPeriod) const {

    auto it = tt.schedules.find(srcClassId);
    if (it == tt.schedules.end()) return {};

    const auto &grid = it->second;
    if (srcDay >= static_cast<int>(grid.size()) ||
        srcPeriod >= static_cast<int>(grid[srcDay].size())) return {};

    const auto &cell = grid[srcDay][srcPeriod];
    if (cell.isEmpty()) return {};

    return checkPlacement(dstClassId, cell.subjectId, cell.teacherId, cell.roomId,
                           dstDay, dstPeriod);
}

bool ConflictChecker::isPlacementValid(int classId, int subjectId, int teacherId,
                                        int roomId, int dayIndex, int periodIndex) const {
    return checkPlacement(classId, subjectId, teacherId, roomId, dayIndex, periodIndex).empty();
}

int ConflictChecker::conflictCount(int classId, int dayIndex, int periodIndex) const {
    auto it = tt.schedules.find(classId);
    if (it == tt.schedules.end()) return 0;
    const auto &grid = it->second;
    if (dayIndex >= static_cast<int>(grid.size()) ||
        periodIndex >= static_cast<int>(grid[dayIndex].size())) return 0;
    const auto &cell = grid[dayIndex][periodIndex];
    if (cell.isEmpty()) return 0;
    return static_cast<int>(checkPlacement(classId, cell.subjectId, cell.teacherId,
                                            cell.roomId, dayIndex, periodIndex).size());
}

std::string ConflictChecker::typeToString(ConflictInfo::Type t) {
    switch (t) {
        case ConflictInfo::Type::TEACHER_BUSY: return "Teacher Busy";
        case ConflictInfo::Type::ROOM_BUSY: return "Room Busy";
        case ConflictInfo::Type::CLASS_BUSY: return "Class Busy";
        case ConflictInfo::Type::TEACHER_UNAVAILABLE: return "Teacher Unavailable";
        case ConflictInfo::Type::FIXED_EVENT: return "Fixed Event";
        case ConflictInfo::Type::SELF_COLLISION: return "Self Collision";
    }
    return "Unknown";
}
