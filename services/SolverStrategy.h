#pragma once
#include "../timetable/Timetable.h"
#include <string>

class DataManager;

struct SolverStats {
    long long nodesVisited = 0;
    int maxRecursionDepth = 0;
    long long prunedBranches = 0;
    double sumDomainSizes = 0.0;
    long long domainChecksCount = 0;
};

struct SolverOptions {
    long long maxStates = 1500000;
    int randomSeed = 0;
    std::string algorithm = "backtrack";
};

class SolverStrategy {
public:
    virtual ~SolverStrategy() = default;
    virtual Timetable solve(const DataManager& dm, SolverStats& stats, const SolverOptions& options = {}) = 0;
    virtual std::string getName() const = 0;
};
