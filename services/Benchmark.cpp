#include "Benchmark.h"
#include "TimetableEngine.h"
#include <iostream>
#include <iomanip>
#include <chrono>

DataManager Benchmark::generateScenario(const std::string& name, int teachers, int classes, int lessonsPerClass) {
    (void)name;
    DataManager dm; // Gets default days (5), periods (8), room types (3)

    // Add teachers
    for (int i = 0; i < teachers; ++i) {
        dm.addTeacher("Teacher_" + std::to_string(i + 1));
    }

    // Add subjects — one per teacher for simplicity
    for (int i = 0; i < teachers; ++i) {
        dm.addSubject("Subject_" + std::to_string(i + 1));
    }

    // Add classes
    for (int i = 0; i < classes; ++i) {
        dm.addClass("Class_" + std::to_string(i + 1), 30 + (i % 10));
    }

    // Add rooms — enough classrooms to support the scenario
    int roomsNeeded = std::max(classes, 3);
    for (int i = 0; i < roomsNeeded; ++i) {
        dm.addRoom("Room_" + std::to_string(i + 1), 45, 1); // Classroom type
    }

    // Add a science lab and computer lab
    dm.addRoom("ScienceLab_1", 40, 2);
    dm.addRoom("CompLab_1", 35, 3);

    // Distribute lessons across classes
    // Each class gets lessonsPerClass lessons, cycling through teachers
    int totalSlots = static_cast<int>(dm.days.size() * dm.periods.size()); // 40
    for (int c = 0; c < classes; ++c) {
        int classId = c + 1;
        int remainingPeriods = std::min(lessonsPerClass * 4, totalSlots); // 4 periods per lesson avg
        int lessonCount = 0;

        for (int t = 0; t < teachers && remainingPeriods > 0; ++t) {
            int teacherId = t + 1;
            int subjectId = t + 1;
            int periodsForThis = std::min(4, remainingPeriods); // 4 periods per lesson
            int maxPerDay = 2;

            dm.addLesson(teacherId, subjectId, classId, periodsForThis, 1, maxPerDay);
            remainingPeriods -= periodsForThis;
            lessonCount++;

            if (lessonCount >= lessonsPerClass) break;
        }
    }

    return dm;
}

BenchmarkResult Benchmark::runSingle(const std::string& name, DataManager& dm) {
    BenchmarkResult result;
    result.scenarioName = name;
    result.numTeachers = static_cast<int>(dm.teachers.size());
    result.numClasses = static_cast<int>(dm.classes.size());
    result.numLessons = static_cast<int>(dm.lessons.size());

    TimetableEngine engine;

    auto startTime = std::chrono::high_resolution_clock::now();
    Timetable timetable = engine.generate(dm);
    auto endTime = std::chrono::high_resolution_clock::now();

    result.executionTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    const auto& stats = engine.getLastRunStats();
    result.statesExplored = stats.nodesVisited;
    result.maxDepth = stats.maxRecursionDepth;
    result.prunedBranches = stats.prunedBranches;
    result.success = timetable.unscheduledLessons.empty();
    result.score = timetable.score;
    result.unscheduledCount = static_cast<int>(timetable.unscheduledLessons.size());

    return result;
}

void Benchmark::printResults(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                  BENCHMARK RESULTS                                                        ║\n";
    std::cout << "╠═══════════════════╦══════════╦═════════╦═════════╦══════════════╦════════════════╦════════╦══════════╦═════════╦══════════════╣\n";
    std::cout << "║ Scenario          ║ Teachers ║ Classes ║ Lessons ║ Time (ms)    ║ States         ║ Depth  ║ Pruned   ║ Score   ║ Status       ║\n";
    std::cout << "╠═══════════════════╬══════════╬═════════╬═════════╬══════════════╬════════════════╬════════╬══════════╬═════════╬══════════════╣\n";

    for (const auto& r : results) {
        std::cout << "║ " << std::left << std::setw(18) << r.scenarioName
                  << "║ " << std::setw(9) << r.numTeachers
                  << "║ " << std::setw(8) << r.numClasses
                  << "║ " << std::setw(8) << r.numLessons
                  << "║ " << std::setw(13) << std::fixed << std::setprecision(2) << r.executionTimeMs
                  << "║ " << std::setw(15) << r.statesExplored
                  << "║ " << std::setw(7) << r.maxDepth
                  << "║ " << std::setw(9) << r.prunedBranches
                  << "║ " << std::setw(8) << r.score
                  << "║ " << std::setw(13) << (r.success ? "✓ Complete" : ("✗ Failed(" + std::to_string(r.unscheduledCount) + ")"))
                  << "║\n";
    }

    std::cout << "╚═══════════════════╩══════════╩═════════╩═════════╩══════════════╩════════════════╩════════╩══════════╩═════════╩══════════════╝\n";
}

void Benchmark::runSuite() {
    std::cout << "\n╔═════════════════════════════════════════╗\n";
    std::cout << "║   TIMETABLE GENERATOR BENCHMARK SUITE   ║\n";
    std::cout << "╚═════════════════════════════════════════╝\n\n";

    struct Scenario {
        std::string name;
        int teachers;
        int classes;
        int lessonsPerClass;
    };

    std::vector<Scenario> scenarios = {
        {"Small",    5,  3,  6},
        {"Medium",  10,  5,  8},
        {"Large",   20, 10, 10},
        {"XLarge",  30, 15, 10},
        {"Extreme", 60, 30, 10}
    };

    std::vector<BenchmarkResult> results;

    for (auto& s : scenarios) {
        std::cout << "Running scenario: " << s.name
                  << " (" << s.teachers << " teachers, " << s.classes << " classes, "
                  << s.lessonsPerClass << " lessons/class)...\n";

        DataManager dm = generateScenario(s.name, s.teachers, s.classes, s.lessonsPerClass);
        BenchmarkResult result = runSingle(s.name, dm);
        results.push_back(result);

        std::cout << "  -> " << (result.success ? "Success" : "Partial") 
                  << " in " << std::fixed << std::setprecision(2) << result.executionTimeMs << " ms\n";
    }

    printResults(results);
}
