#pragma once
#include "DataManager.h"
#include <vector>
#include <string>

struct BenchmarkResult {
    std::string scenarioName;
    int numTeachers;
    int numClasses;
    int numLessons;
    double executionTimeMs;
    long long statesExplored;
    int maxDepth;
    long long prunedBranches;
    bool success;
    int score;
    int unscheduledCount;
};

class Benchmark {
public:
    static void runSuite();

private:
    static DataManager generateScenario(const std::string& name, int teachers, int classes, int lessonsPerClass);
    static BenchmarkResult runSingle(const std::string& name, DataManager& dm);
    static void printResults(const std::vector<BenchmarkResult>& results);
};
