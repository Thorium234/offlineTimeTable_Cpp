#pragma once
#include "SolverStrategy.h"
#include "ResourceTracker.h"
#include "../models/LessonUnit.h"
#include <vector>

class GreedySolver : public SolverStrategy {
private:
    int countSubjectOnDay(const Timetable& timetable, int classId, int subjectId, 
                          int dayIdx, int numPeriods) const;

public:
    Timetable solve(const DataManager& dm, SolverStats& stats) override;
    std::string getName() const override { return "Greedy Fallback Solver"; }
};
