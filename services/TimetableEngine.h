#pragma once
#include "DataManager.h"
#include "../timetable/Timetable.h"
#include "SolverStrategy.h"

class TimetableEngine {
private:
    SolverStats lastRunStats;

public:
    Timetable generate(const DataManager& dm);
    Timetable generateWithStrategy(const DataManager& dm, SolverStrategy& strategy);

    long long getStatesVisited() const { return lastRunStats.nodesVisited; }
    const SolverStats& getLastRunStats() const { return lastRunStats; }
};
