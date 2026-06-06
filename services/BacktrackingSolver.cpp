#include "BacktrackingSolver.h"
#include "DataManager.h"
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

int BacktrackingSolver::countTeacherConsecutive(const ResourceTracker& tracker, int teacherId,
                                                int dayIdx, int numPeriods) const {
    int maxConsecutive = 0;
    int currentConsecutive = 0;
    
    for (int p = 0; p < numPeriods; ++p) {
        if (tracker.isBusy(ResourceType::TEACHER, teacherId, dayIdx, p)) {
            currentConsecutive++;
            maxConsecutive = std::max(maxConsecutive, currentConsecutive);
        } else {
            currentConsecutive = 0;
        }
    }
    
    return maxConsecutive;
}

int BacktrackingSolver::getTeacherMaxConsecutive(const DataManager& dm, int teacherId) const {
    for (const auto& t : dm.teachers) {
        if (t.id == teacherId) {
            return t.maxConsecutive;
        }
    }
    return 0; // No limit by default
}

int BacktrackingSolver::getClassEarliestPeriod(const DataManager& dm, int classId) const {
    for (const auto& c : dm.classes) {
        if (c.id == classId) {
            return c.earliestPeriod;
        }
    }
    return -1; // No constraint
}

int BacktrackingSolver::getClassLatestPeriod(const DataManager& dm, int classId) const {
    for (const auto& c : dm.classes) {
        if (c.id == classId) {
            return c.latestPeriod;
        }
    }
    return -1; // No constraint
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

        // Conflicts if sharing any class or teacher
        bool sharesClass = (other.classId == placedUnit.classId);
        for (int ccid : placedUnit.combinedClassIds) {
            if (other.classId == ccid) { sharesClass = true; break; }
        }
        bool sharesTeacher = (other.teacherId == placedUnit.teacherId ||
                             (placedUnit.secondTeacherId >= 0 && other.teacherId == placedUnit.secondTeacherId));

        if (sharesClass || sharesTeacher) {
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

    if (stats.nodesVisited > m_maxStates) {
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

    // Value ordering using Least Constraining Value (LCV) + gap-awareness
    struct ValueCandidate {
        int d, p;
        int lcvScore;
        int gapCost;
    };
    std::vector<ValueCandidate> candidates;
    const auto& domain = domainTracker.getDomain(chosenUnitIdx);
    for (const auto& slot : domain) {
        int lcv = getLCVScore(chosenUnitIdx, slot.first, slot.second, units, placed, domainTracker, numPeriods);

        // Soft score: estimate gap cost if placed here for the teacher (week-aware)
        int gapCost = 0;
        int wt = unit.weekType;
        int before = (slot.second > 0 && tracker.isBusy(ResourceType::TEACHER, unit.teacherId, slot.first, slot.second - 1, wt)) ? 0 : 1;
        int afterBlock = slot.second + k;
        int after = (afterBlock < numPeriods && tracker.isBusy(ResourceType::TEACHER, unit.teacherId, slot.first, afterBlock, wt)) ? 0 : 1;
        gapCost = before + after;

        candidates.push_back({slot.first, slot.second, lcv, gapCost});
    }

    // Sort by LCV first (hard constraint), then gap cost (soft optimization)
    std::sort(candidates.begin(), candidates.end(), [](const ValueCandidate& a, const ValueCandidate& b) {
        if (a.lcvScore != b.lcvScore) return a.lcvScore < b.lcvScore;
        return a.gapCost < b.gapCost; // prefer placements that fill/avoid gaps
    });

    for (const auto& cand : candidates) {
        // Build all class and teacher IDs
        std::vector<int> allClassIds = {unit.classId};
        allClassIds.insert(allClassIds.end(), unit.combinedClassIds.begin(), unit.combinedClassIds.end());
        std::vector<int> allTeacherIds = {unit.teacherId};
        if (unit.secondTeacherId >= 0) allTeacherIds.push_back(unit.secondTeacherId);

        int d = cand.d;
        int p = cand.p;

        // Check if all slots in the block are free (week-aware, multi-class, multi-teacher)
        int wt = unit.weekType;
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
                        blocked = (fe.periodId == periodId);
                        break;
                    case RecurrenceType::WEEKLY:
                    case RecurrenceType::NONE:
                        blocked = (fe.dayId == dayId && fe.periodId == periodId);
                        break;
                    default:
                        throw std::logic_error("Unknown recurrence type");
                }
                if (blocked) { canPlace = false; break; }
            }
            if (!canPlace) break;

            // Check all classes
            for (int cid : allClassIds) {
                if (tracker.isBusy(ResourceType::CLASS, cid, d, currentP, wt)) {
                    canPlace = false; break;
                }
            }
            if (!canPlace) break;

            // Check all teachers
            for (int tid : allTeacherIds) {
                if (tracker.isBusy(ResourceType::TEACHER, tid, d, currentP, wt) ||
                    dm.isTeacherUnavailable(tid, dayId, periodId)) {
                    canPlace = false; break;
                }
            }
            if (!canPlace) break;
        }
        if (!canPlace) continue;

        // ==== Check Education Block (Earliest/Latest Period) ====
        int earliestPeriod = getClassEarliestPeriod(dm, unit.classId);
        int latestPeriod = getClassLatestPeriod(dm, unit.classId);

        if (earliestPeriod >= 0 && p < earliestPeriod) {
            continue;
        }
        if (latestPeriod >= 0 && p + k - 1 > latestPeriod) {
            continue;
        }

        // ==== Check Teacher Max Consecutive for both teachers (week-aware) ====
        bool consecOk = true;
        for (int tid : allTeacherIds) {
            int tmc = getTeacherMaxConsecutive(dm, tid);
            if (tmc > 0) {
                int left = 0;
                for (int i = p - 1; i >= 0 && tracker.isBusy(ResourceType::TEACHER, tid, d, i, wt); --i) {
                    left++;
                }
                int right = 0;
                int afterBlock = p + k;
                for (int i = afterBlock; i < numPeriods && tracker.isBusy(ResourceType::TEACHER, tid, d, i, wt); ++i) {
                    right++;
                }
                int totalConsecutive = left + k + right;
                if (totalConsecutive > tmc) { consecOk = false; break; }
            }
        }
        if (!consecOk) continue;

        // Check maxPerDay constraint
        if (unit.maxPerDay > 0) {
            int alreadyOnDay = countSubjectOnDay(timetable, unit.classId, unit.subjectId, d, numPeriods);
            if (alreadyOnDay + k > unit.maxPerDay) continue;
        }

        // Find a room available (week-aware)
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
            // Apply placement (multi-class, multi-teacher, week-aware)
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
            placed[chosenUnitIdx] = true;
            assignedSlots[chosenUnitIdx] = d * numPeriods + p;

            auto domainsSnapshot = domainTracker.getSnapshot();

            bool feasible = domainTracker.propagate(chosenUnitIdx, d, p, units, placed, numPeriods);

            if (feasible) {
                if (backtrack(placedCount + 1, units, placed, dm, tracker, domainTracker,
                              timetable, assignedSlots, numDays, numPeriods, stats, depth + 1)) {
                    return true;
                }
            } else {
                stats.prunedBranches++;
            }

            domainTracker.restoreSnapshot(domainsSnapshot);

            // Undo placement (multi-class, multi-teacher, week-aware)
            for (int offset = 0; offset < k; ++offset) {
                for (int cid : allClassIds) {
                    timetable.clearSlot(cid, d, p + offset);
                    tracker.markBusy(ResourceType::CLASS, cid, d, p + offset, false);
                }
                for (int tid : allTeacherIds) {
                    tracker.markBusy(ResourceType::TEACHER, tid, d, p + offset, false);
                }
                tracker.markBusy(ResourceType::ROOM, allocatedRoomId, d, p + offset, false);
            }
            placed[chosenUnitIdx] = false;
            assignedSlots[chosenUnitIdx] = -1;
        }
    }

    return false;
}

Timetable BacktrackingSolver::solve(const DataManager& dm, SolverStats& stats, const SolverOptions& options) {
    m_maxStates = options.maxStates;
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
