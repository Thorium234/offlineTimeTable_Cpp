#include "BacktrackingSolver.h"
#include "TimetableEvaluator.h"
#include "FeasibilityChecker.h"
#include <iostream>
#include <algorithm>
#include <map>
#include <stdexcept>

int BacktrackingSolver::countSubjectOnDay(const Timetable& timetable, int classId, int subjectId,
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

int BacktrackingSolver::getLCVScore(int unitIdx, int d, int p,
                                     const std::vector<LessonUnit>& units,
                                     const std::vector<bool>& placed,
                                     const DomainTracker& domainTracker,
                                     int numPeriods) const {
    (void)numPeriods;
    int eliminatedCount = 0;
    const auto& placedUnit = units[unitIdx];
    int k = placedUnit.blockSize;

    for (size_t u = 0; u < units.size(); ++u) {
        if (placed[u] || static_cast<int>(u) == unitIdx) {
            continue;
        }

        const auto& other = units[u];

        // Only overlaps/conflicts if sharing class or teacher
        if (other.classId == placedUnit.classId || other.teacherId == placedUnit.teacherId) {
            int k_other = other.blockSize;
            const auto& dom = domainTracker.getDomain(u);
            for (const auto& slot : dom) {
                if (slot.first == d) {
                    int p_other = slot.second;
                    // Check overlap
                    if (p_other <= p + k - 1 && p_other + k_other - 1 >= p) {
                        eliminatedCount++;
                    }
                }
            }
        }
    }

    return eliminatedCount;
}

bool BacktrackingSolver::backtrack(int placedCount,
                                   const std::vector<LessonUnit>& units,
                                   std::vector<bool>& placed,
                                   const DataManager& dm,
                                   ResourceTracker& tracker,
                                   DomainTracker& domainTracker,
                                   Timetable& timetable,
                                   std::vector<int>& assignedSlots,
                                   int numDays,
                                   int numPeriods,
                                   SolverStats& stats,
                                   int depth) {
    if (placedCount == static_cast<int>(units.size())) {
        return true;
    }

    stats.nodesVisited++;
    if (depth > stats.maxRecursionDepth) {
        stats.maxRecursionDepth = depth;
    }

    if (stats.nodesVisited > MAX_STATES) {
        return false;
    }

    // Dynamic selection of the next variable (MRV heuristic)
    int chosenUnitIdx = -1;
    size_t minDomainSize = 9999999;

    for (size_t i = 0; i < units.size(); ++i) {
        if (!placed[i]) {
            size_t domSize = domainTracker.getDomain(i).size();
            stats.sumDomainSizes += domSize;
            stats.domainChecksCount++;

            if (domSize < minDomainSize) {
                minDomainSize = domSize;
                chosenUnitIdx = i;
            }
        }
    }

    // If a variable has no valid values left in its domain, backtrack
    if (chosenUnitIdx == -1 || minDomainSize == 0) {
        stats.prunedBranches++;
        return false;
    }

    const auto& unit = units[chosenUnitIdx];
    int k = unit.blockSize;

    // Value ordering using Least Constraining Value (LCV)
    struct ValueCandidate {
        int d, p;
        int lcvScore;
    };
    std::vector<ValueCandidate> candidates;
    const auto& domain = domainTracker.getDomain(chosenUnitIdx);
    for (const auto& slot : domain) {
        int lcv = getLCVScore(chosenUnitIdx, slot.first, slot.second, units, placed, domainTracker, numPeriods);
        candidates.push_back({slot.first, slot.second, lcv});
    }

    // Sort by LCV score ascending (least constraining first)
    std::sort(candidates.begin(), candidates.end(), [](const ValueCandidate& a, const ValueCandidate& b) {
        return a.lcvScore < b.lcvScore;
    });

    for (const auto& cand : candidates) {
        int d = cand.d;
        int p = cand.p;

        // Check if all slots in the block are free
        bool canPlace = true;
        for (int offset = 0; offset < k; ++offset) {
            int currentP = p + offset;
            int dayId = dm.days[d].id;
            int periodId = dm.periods[currentP].id;

            // Fixed events conflict check (recurrence-aware)
            for (const auto& fe : dm.fixedEvents) {
                bool blocked = false;
                switch (fe.recurrence) {
                    case RecurrenceType::DAILY:
                        // Blocks this period on ALL days
                        blocked = (fe.periodId == periodId);
                        break;
                    case RecurrenceType::WEEKLY:
                    case RecurrenceType::NONE:
                        // Blocks only specific (day, period)
                        blocked = (fe.dayId == dayId && fe.periodId == periodId);
                        break;
                    default:
                        throw std::logic_error("Unknown recurrence type");
                }
                if (blocked) {
                    canPlace = false;
                    break;
                }
            }
            if (!canPlace) break;

            // Class, Teacher, or unavailability checks
            if (tracker.isBusy(ResourceType::CLASS, unit.classId, d, currentP) ||
                tracker.isBusy(ResourceType::TEACHER, unit.teacherId, d, currentP) ||
                dm.isTeacherUnavailable(unit.teacherId, dayId, periodId)) {
                canPlace = false;
                break;
            }
        }
        if (!canPlace) continue;

        // Check maxPerDay constraint
        if (unit.maxPerDay > 0) {
            int alreadyOnDay = countSubjectOnDay(timetable, unit.classId, unit.subjectId, d, numPeriods);
            if (alreadyOnDay + k > unit.maxPerDay) continue;
        }

        // Find a room available and of sufficient capacity for all periods of the block
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
            // Apply placement
            for (int offset = 0; offset < k; ++offset) {
                timetable.setSlot(unit.classId, d, p + offset, unit.subjectId, unit.teacherId, allocatedRoomId);
                tracker.markBusy(ResourceType::CLASS, unit.classId, d, p + offset, true);
                tracker.markBusy(ResourceType::TEACHER, unit.teacherId, d, p + offset, true);
                tracker.markBusy(ResourceType::ROOM, allocatedRoomId, d, p + offset, true);
            }
            placed[chosenUnitIdx] = true;
            assignedSlots[chosenUnitIdx] = d * numPeriods + p;

            // Snapshot domains
            auto domainsSnapshot = domainTracker.getSnapshot();

            // Propagate constraints
            bool feasible = domainTracker.propagate(chosenUnitIdx, d, p, units, placed, numPeriods);

            if (feasible) {
                if (backtrack(placedCount + 1, units, placed, dm, tracker, domainTracker, 
                              timetable, assignedSlots, numDays, numPeriods, stats, depth + 1)) {
                    return true;
                }
            } else {
                stats.prunedBranches++;
            }

            // Restore domains snapshot
            domainTracker.restoreSnapshot(domainsSnapshot);

            // Undo placement
            for (int offset = 0; offset < k; ++offset) {
                timetable.clearSlot(unit.classId, d, p + offset);
                tracker.markBusy(ResourceType::CLASS, unit.classId, d, p + offset, false);
                tracker.markBusy(ResourceType::TEACHER, unit.teacherId, d, p + offset, false);
                tracker.markBusy(ResourceType::ROOM, allocatedRoomId, d, p + offset, false);
            }
            placed[chosenUnitIdx] = false;
            assignedSlots[chosenUnitIdx] = -1;
        }
    }

    return false;
}

Timetable BacktrackingSolver::solve(const DataManager& dm, SolverStats& stats) {
    Timetable timetable;
    int numDays = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());

    // Initialize tracking structures
    ResourceTracker tracker(numDays, numPeriods);
    for (const auto& t : dm.teachers) tracker.initResource(ResourceType::TEACHER, t.id);
    for (const auto& r : dm.rooms) tracker.initResource(ResourceType::ROOM, r.id);
    for (const auto& c : dm.classes) tracker.initResource(ResourceType::CLASS, c.id);

    for (const auto& c : dm.classes) {
        timetable.initClass(c.id, numDays, numPeriods);
    }

    // Partition all lessons into block-sized units
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

    // Initialize DomainTracker with fixed events for recurrence-aware pruning
    DomainTracker domainTracker;
    if (!dm.fixedEvents.empty()) {
        domainTracker.initWithFixedEvents(static_cast<int>(units.size()), numDays, numPeriods, units, dm.fixedEvents);
    } else {
        domainTracker.init(static_cast<int>(units.size()), numDays, numPeriods, units);
    }

    std::vector<bool> placed(units.size(), false);
    std::vector<int> assignedSlots(units.size(), -1);

    bool success = backtrack(0, units, placed, dm, tracker, domainTracker, 
                             timetable, assignedSlots, numDays, numPeriods, stats, 0);

    if (success) {
        TimetableEvaluator evaluator;
        timetable.score = evaluator.calculateScore(timetable, dm);
    } else {
        timetable.score = 0; // Solver failed to find a valid complete solution
    }

    return timetable;
}
