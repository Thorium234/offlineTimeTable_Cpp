#pragma once
#include "SolverStrategy.h"
#include "ResourceTracker.h"
#include "DomainTracker.h"
#include <vector>

class BacktrackingSolver : public SolverStrategy {
private:
    const long long MAX_STATES = 1500000; // Large search space limit for Extreme/Large scales

    int countSubjectOnDay(const Timetable& timetable, int classId, int subjectId, 
                          int dayIdx, int numPeriods) const;

    bool backtrack(int placedCount,
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
                   int depth);

    int getLCVScore(int unitIdx, int d, int p,
                    const std::vector<LessonUnit>& units,
                    const std::vector<bool>& placed,
                    const DomainTracker& domainTracker,
                    int numPeriods) const;

public:
    Timetable solve(const DataManager& dm, SolverStats& stats) override;
    std::string getName() const override { return "Advanced Backtracking Solver (MRV + LCV + Domain Propagation)"; }
};
