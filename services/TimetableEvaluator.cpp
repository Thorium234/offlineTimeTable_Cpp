#include "TimetableEvaluator.h"
#include <map>
#include <vector>
#include <algorithm>
#include "../models/TeacherPreference.h"

int TimetableEvaluator::calculateScore(const Timetable& timetable, const DataManager& dm) const {
    int score = 1000;
    int numDays = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());

    // 1. Class Gaps
    for (const auto& pair : timetable.schedules) {
        const auto& grid = pair.second;
        for (int d = 0; d < numDays; ++d) {
            int firstPeriod = -1;
            int lastPeriod = -1;
            for (int p = 0; p < numPeriods; ++p) {
                if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                    if (!grid[d][p].isEmpty()) {
                        if (firstPeriod == -1) firstPeriod = p;
                        lastPeriod = p;
                    }
                }
            }
            if (firstPeriod != -1 && firstPeriod < lastPeriod) {
                int currentGapRun = 0;
                for (int p = firstPeriod; p <= lastPeriod; ++p) {
                    if (grid[d][p].isEmpty()) {
                        currentGapRun++;
                    } else {
                        if (currentGapRun > 0) {
                            score -= weights.classGapWeight * currentGapRun * currentGapRun;
                            currentGapRun = 0;
                        }
                    }
                }
                if (currentGapRun > 0) {
                    score -= weights.classGapWeight * currentGapRun * currentGapRun;
                }
            }
        }
    }

    // 2. Teacher Gaps
    std::map<int, std::vector<std::vector<bool>>> teacherActive;
    for (const auto& t : dm.teachers) {
        teacherActive[t.id] = std::vector<std::vector<bool>>(numDays, std::vector<bool>(numPeriods, false));
    }
    for (const auto& pair : timetable.schedules) {
        const auto& grid = pair.second;
        for (int d = 0; d < numDays; ++d) {
            for (int p = 0; p < numPeriods; ++p) {
                if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                    if (!grid[d][p].isEmpty()) {
                        teacherActive[grid[d][p].teacherId][d][p] = true;
                    }
                }
            }
        }
    }

    for (const auto& t : dm.teachers) {
        const auto& grid = teacherActive[t.id];
        for (int d = 0; d < numDays; ++d) {
            int firstPeriod = -1;
            int lastPeriod = -1;
            for (int p = 0; p < numPeriods; ++p) {
                if (grid[d][p]) {
                    if (firstPeriod == -1) firstPeriod = p;
                    lastPeriod = p;
                }
            }
            if (firstPeriod != -1 && firstPeriod < lastPeriod) {
                int currentGapRun = 0;
                for (int p = firstPeriod; p <= lastPeriod; ++p) {
                    if (!grid[d][p]) {
                        currentGapRun++;
                    } else {
                        if (currentGapRun > 0) {
                            score -= weights.teacherGapWeight * currentGapRun * currentGapRun;
                            currentGapRun = 0;
                        }
                    }
                }
                if (currentGapRun > 0) {
                    score -= weights.teacherGapWeight * currentGapRun * currentGapRun;
                }
            }
        }
    }

    // 3. Subject Clustering Penalty (Sprint 3)
    // Penalize if any subject appears 3+ times on a single day for a class
    for (const auto& pair : timetable.schedules) {
        (void)pair.first; // classId used implicitly through grid
        const auto& grid = pair.second;
        for (int d = 0; d < numDays; ++d) {
            std::map<int, int> subjectCount;
            for (int p = 0; p < numPeriods; ++p) {
                if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                    if (!grid[d][p].isEmpty()) {
                        subjectCount[grid[d][p].subjectId]++;
                    }
                }
            }
            for (const auto& sc : subjectCount) {
                if (sc.second >= 3) {
                    // Heavy penalty for 3+ same subject in one day
                    score -= weights.subjectClusterWeight * (sc.second - 2);
                }
            }
        }
    }

    // 4. Unscheduled Lessons Penalty
    score -= static_cast<int>(timetable.unscheduledLessons.size()) * weights.unscheduledWeight;

    // 5. Teacher Preferences
    for (const auto& pair : timetable.schedules) {
        const auto& grid = pair.second;
        for (int d = 0; d < numDays; ++d) {
            for (int p = 0; p < numPeriods; ++p) {
                if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                    const auto& cell = grid[d][p];
                    if (!cell.isEmpty()) {
                        int dayId = dm.days[d].id;
                        int periodId = dm.periods[p].id;
                        PreferenceType pref = dm.getTeacherPreference(cell.teacherId, dayId, periodId);
                        if (pref == PreferenceType::PREFERRED) {
                            score += weights.teacherPreferredBonus;
                        } else if (pref == PreferenceType::UNDESIRABLE) {
                            score -= weights.teacherUndesirablePenalty;
                        }
                    }
                }
            }
        }
    }

    if (score < 0) score = 0;
    return score;
}
