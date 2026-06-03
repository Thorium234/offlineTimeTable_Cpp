#include "GreedySolver.h"
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

Timetable GreedySolver::solve(const DataManager& dm, SolverStats& stats) {
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
                lesson.subjectId,
                lesson.classId,
                reqRoomTypeId,
                studentCount,
                curSize,
                lesson.maxPerDay
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
        stats.nodesVisited++; // Treat each placement attempt as a visited node/decision

        bool placed = false;
        int k = unit.blockSize;

        // Try to place the block in the first matching slot
        for (int d = 0; d < numDays && !placed; ++d) {
            // Check maxPerDay constraint
            if (unit.maxPerDay > 0) {
                int alreadyOnDay = countSubjectOnDay(timetable, unit.classId, unit.subjectId, d, numPeriods);
                if (alreadyOnDay + k > unit.maxPerDay) continue;
            }

            for (int p = 0; p <= numPeriods - k && !placed; ++p) {
                // Check if teacher/class is busy
                bool slotFree = true;
                for (int offset = 0; offset < k; ++offset) {
                    int dayId = dm.days[d].id;
                    int periodId = dm.periods[p + offset].id;

                    // Fixed events conflict check (recurrence-aware)
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
                        if (blocked) {
                            slotFree = false;
                            break;
                        }
                    }
                    if (!slotFree) break;

                    if (tracker.isBusy(ResourceType::CLASS, unit.classId, d, p + offset) ||
                        tracker.isBusy(ResourceType::TEACHER, unit.teacherId, d, p + offset) ||
                        dm.isTeacherUnavailable(unit.teacherId, dayId, periodId)) {
                        slotFree = false;
                        break;
                    }
                }
                if (!slotFree) continue;

                // Find suitable room
                int allocatedRoomId = -1;
                for (const auto& room : dm.rooms) {
                    if (room.roomTypeId == unit.reqRoomTypeId && room.capacity >= unit.studentCount) {
                        bool roomFree = true;
                        for (int offset = 0; offset < k; ++offset) {
                            if (tracker.isBusy(ResourceType::ROOM, room.id, d, p + offset)) {
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
                    // Place it!
                    for (int offset = 0; offset < k; ++offset) {
                        timetable.setSlot(unit.classId, d, p + offset, unit.subjectId, unit.teacherId, allocatedRoomId);
                        tracker.markBusy(ResourceType::CLASS, unit.classId, d, p + offset, true);
                        tracker.markBusy(ResourceType::TEACHER, unit.teacherId, d, p + offset, true);
                        tracker.markBusy(ResourceType::ROOM, allocatedRoomId, d, p + offset, true);
                    }
                    placed = true;
                }
            }
        }


        if (!placed) {
            // Simple explanation without specific day/period because they are out of scope here
            std::string reason = "Greedy Placement Failed: No free consecutive slots of size " +
                                 std::to_string(k) + " found with teacher/class/room available.";
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
