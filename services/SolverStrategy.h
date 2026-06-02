#pragma once
#include "../timetable/Timetable.h"
#include "DataManager.h"
#include <string>

struct SolverStats {
    long long nodesVisited = 0;
    int maxRecursionDepth = 0;
    long long prunedBranches = 0;
    double sumDomainSizes = 0.0;
    long long domainChecksCount = 0;
};

class SolverStrategy {
public:
    virtual ~SolverStrategy() = default;
    virtual Timetable solve(const DataManager& dm, SolverStats& stats) = 0;
    virtual std::string getName() const = 0;
};
