#include "GreedySolver.h"
#include "DataManager.h"
#include "TimetableEvaluator.h"
#include <map>
#include <algorithm>
#include <iostream>
#include <stdexcept>

int GreedySolver::countSubjectOnDay(const Timetable& timetable, int classId, int subjectId,
                                     int dayIdx, int numPeriods) const {
    int count = 0;
    for (int p = 0; p < numPeriods; ++p) {
        auto cell = timetable.getSlot(classId, dayIdx, p);
        if (cell.subjectId == subjectId) {
            count++;
        }
    }
    return count;
}

Timetable GreedySolver::solve(const DataManager& dm, SolverStats& stats, const SolverOptions& options) {
    Timetable timetable;
    int numDays = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());

    // Initialize ResourceTracker
    ResourceTracker tracker(numDays, numPeriods);
    for (const auto& t : dm.teachers) tracker.initResource(ResourceType::TEACHER, t.id);
    for (const auto& r : dm.rooms) tracker.initResource(ResourceType::ROOM, r.id);
    for (const auto& c : dm.classes) tracker.initResource(ResourceType::CLASS, c.id);

    for (const auto& c : dm.classes) {
        timetable.initClass(c.id, numDays, numPeriods);
    }

    // Partition all lessons into block units
    std::vector<LessonUnit> units;
    for (size_t i = 0; i < dm.lessons.size(); ++i) {
        const auto& lesson = dm.lessons[i];
        int reqRoomTypeId = dm.getSubjectRequiredRoomTypeId(lesson.subjectId);
        int studentCount = 0;
        for (const auto& c : dm.classes) {
            if (c.id == lesson.classId) {
                studentCount = c.studentCount;
                break;
            }
        }

        int periodsLeft = lesson.periodsPerWeek;
        while (periodsLeft > 0) {
            int curSize = std::min(periodsLeft, lesson.blockSize);
            units.push_back(LessonUnit{
                static_cast<int>(i),
                lesson.teacherId,
                lesson.secondTeacherId,
                lesson.subjectId,
                lesson.classId,
                lesson.combinedClassIds,
                reqRoomTypeId,
                studentCount,
                curSize,
                lesson.maxPerDay,
                lesson.weekType
            });
            periodsLeft -= curSize;
        }
    }

    // Sort units: larger block sizes first, then higher periods per week of the lesson
    std::sort(units.begin(), units.end(), [](const LessonUnit& a, const LessonUnit& b) {
        if (a.blockSize != b.blockSize) {
            return a.blockSize > b.blockSize; // Larger blocks first
        }
        return a.studentCount > b.studentCount; // Tie-breaker: larger class size first
    });

    for (const auto& unit : units) {
        stats.nodesVisited++;

        bool placed = false;
        int k = unit.blockSize;

        // Build list of all class IDs (primary + combined)
        std::vector<int> allClassIds;
        allClassIds.push_back(unit.classId);
        allClassIds.insert(allClassIds.end(), unit.combinedClassIds.begin(), unit.combinedClassIds.end());

        // Build list of all teacher IDs (primary + second)
        std::vector<int> allTeacherIds;
        allTeacherIds.push_back(unit.teacherId);
        if (unit.secondTeacherId >= 0) {
            allTeacherIds.push_back(unit.secondTeacherId);
        }

        // Try to place the block in the first matching slot
        for (int d = 0; d < numDays && !placed; ++d) {
            // Check maxPerDay constraint for primary class
            if (unit.maxPerDay > 0) {
                int alreadyOnDay = countSubjectOnDay(timetable, unit.classId, unit.subjectId, d, numPeriods);
                if (alreadyOnDay + k > unit.maxPerDay) continue;
            }

            for (int p = 0; p <= numPeriods - k && !placed; ++p) {
                bool slotFree = true;
                int wt = unit.weekType;
                for (int offset = 0; offset < k; ++offset) {
                    int dayId = dm.days[d].id;
                    int periodId = dm.periods[p + offset].id;

                    // Fixed events conflict check
                    for (const auto& fe : dm.fixedEvents) {
                        bool blocked = false;
                        switch (fe.recurrence) {
                            case RecurrenceType::DAILY:
                                blocked = (fe.periodId == periodId);
                                break;
                            case RecurrenceType::WEEKLY:
                            case RecurrenceType::NONE:
                                blocked = (fe.dayId == dayId && fe.periodId == periodId);
                                break;
                            default:
                                throw std::logic_error("Unknown recurrence type");
                        }
                        if (blocked) { slotFree = false; break; }
                    }
                    if (!slotFree) break;

                    // Check all classes (primary + combined)
                    for (int cid : allClassIds) {
                        if (tracker.isBusy(ResourceType::CLASS, cid, d, p + offset, wt)) {
                            slotFree = false; break;
                        }
                    }
                    if (!slotFree) break;

                    // Check all teachers (primary + second)
                    for (int tid : allTeacherIds) {
                        if (tracker.isBusy(ResourceType::TEACHER, tid, d, p + offset, wt) ||
                            dm.isTeacherUnavailable(tid, dayId, periodId)) {
                            slotFree = false; break;
                        }
                    }
                    if (!slotFree) break;
                }
                if (!slotFree) continue;

                // Find suitable room
                int allocatedRoomId = -1;
                for (const auto& room : dm.rooms) {
                    if (room.roomTypeId == unit.reqRoomTypeId && room.capacity >= unit.studentCount) {
                        bool roomFree = true;
                for (int offset = 0; offset < k; ++offset) {
                    if (tracker.isBusy(ResourceType::ROOM, room.id, d, p + offset, wt)) {
                                roomFree = false;
                                break;
                            }
                        }
                        if (roomFree) {
                            allocatedRoomId = room.id;
                            break;
                        }
                    }
                }

                if (allocatedRoomId != -1) {
                    // Place it! Mark all classes and teachers as busy
                    for (int offset = 0; offset < k; ++offset) {
                        for (int cid : allClassIds) {
                            timetable.setSlot(cid, d, p + offset, unit.subjectId, unit.teacherId, allocatedRoomId, wt);
                            tracker.markBusy(ResourceType::CLASS, cid, d, p + offset, wt);
                        }
                        for (int tid : allTeacherIds) {
                            tracker.markBusy(ResourceType::TEACHER, tid, d, p + offset, wt);
                        }
                        tracker.markBusy(ResourceType::ROOM, allocatedRoomId, d, p + offset, wt);
                    }
                    placed = true;
                }
            }
        }


        if (!placed) {
            // Build class names for error message
            std::string classList = dm.getClassName(unit.classId);
            for (int ccid : unit.combinedClassIds) {
                classList += "+" + dm.getClassName(ccid);
            }
            std::string reason = "Greedy Placement Failed: No free consecutive slots of size " +
                                 std::to_string(k) + " found for " + classList +
                                 " with teacher/class/room available.";
            dm.logPlacementReject(reason);
            timetable.unscheduledLessons.push_back(UnscheduledLesson{
                unit.subjectId,
                unit.classId,
                unit.teacherId,
                k,
                reason
            });
        }

    }

    TimetableEvaluator evaluator;
    timetable.score = evaluator.calculateScore(timetable, dm);

    return timetable;
}
