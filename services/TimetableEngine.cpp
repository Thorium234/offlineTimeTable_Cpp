#include "TimetableEngine.h"
#include "FeasibilityChecker.h"
#include "BacktrackingSolver.h"
#include "GreedySolver.h"
#include <iostream>

Timetable TimetableEngine::generate(const DataManager& dm) {
    // Default to the advanced BacktrackingSolver
    BacktrackingSolver defaultSolver;
    return generateWithStrategy(dm, defaultSolver);
}

Timetable TimetableEngine::generateWithStrategy(const DataManager& dm, SolverStrategy& strategy) {
    std::cout << "\n========================================================================\n";
    std::cout << "Starting Generation using: " << strategy.getName() << "\n";
    std::cout << "========================================================================\n";

    int numDays = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());
    Timetable emptyTimetable;

    if (numDays == 0 || numPeriods == 0) {
        std::cout << "[Error] Cannot generate timetable: Days or Periods are not defined.\n";
        return emptyTimetable;
    }

    // Pre-validation
    FeasibilityChecker checker;
    auto feasErrors = checker.check(dm);
    if (!feasErrors.empty()) {
        std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║              FEASIBILITY CHECK FAILED                        ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
        std::cout << "The following issues make scheduling IMPOSSIBLE:\n\n";
        for (size_t i = 0; i < feasErrors.size(); ++i) {
            std::cout << "  [" << feasErrors[i].category << "] " << feasErrors[i].message << "\n";
        }
        std::cout << "\nPlease fix these issues before attempting to generate a timetable.\n";
        return emptyTimetable;
    }
    std::cout << "[✓] Pre-validation passed. Schedule is feasible.\n";

    // Reset stats
    lastRunStats = SolverStats();

    // Execute Strategy
    Timetable result = strategy.solve(dm, lastRunStats, dm.solverOptions);

    // If backtracking failed completely, provide a greedy fallback (optional, but good for UX)
    if (result.score == 0 && lastRunStats.nodesVisited > 0 && dynamic_cast<BacktrackingSolver*>(&strategy)) {
        std::cout << "[Warning] Primary solver failed to find a complete solution. Attempting Greedy Fallback...\n";
        GreedySolver fallbackSolver;
        SolverStats fallbackStats;
        result = fallbackSolver.solve(dm, fallbackStats, dm.solverOptions);
        
        // Accumulate stats for transparency
        lastRunStats.nodesVisited += fallbackStats.nodesVisited;
    }

    return result;
}
