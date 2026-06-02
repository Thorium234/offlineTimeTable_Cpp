#include "Timetable.h"
#include "../services/DataManager.h"
#include <iostream>
#include <iomanip>

const std::string DAY_NAMES[DAYS] = {"MON", "TUE", "WED", "THU", "FRI"};

void Timetable::initClass(int classId) {
    schedules[classId] = std::vector<std::vector<std::string>>(DAYS, std::vector<std::string>(PERIODS, "---"));
}

void Timetable::setSlot(int classId, int day, int period, const std::string& subjectName) {
    if (schedules.find(classId) == schedules.end()) {
        initClass(classId);
    }
    schedules[classId][day][period] = subjectName;
}

std::string Timetable::getSlot(int classId, int day, int period) const {
    auto it = schedules.find(classId);
    if (it != schedules.end()) {
        return it->second[day][period];
    }
    return "---";
}

void Timetable::print(const DataManager& dm) const {
    if (schedules.empty()) {
        std::cout << "\nNo timetable generated yet or no classes exist.\n";
        return;
    }

    for (const auto& pair : schedules) {
        int classId = pair.first;
        const auto& grid = pair.second;

        std::cout << "\n=================================================================================\n";
        std::cout << " CLASS: " << dm.getClassName(classId) << "\n";
        std::cout << "=================================================================================\n";

        // Print header
        std::cout << std::left << std::setw(8) << "";
        for (int p = 0; p < PERIODS; ++p) {
            std::cout << std::left << std::setw(10) << ("P" + std::to_string(p + 1));
        }
        std::cout << "\n---------------------------------------------------------------------------------\n";

        // Print days
        for (int d = 0; d < DAYS; ++d) {
            std::cout << std::left << std::setw(8) << DAY_NAMES[d];
            for (int p = 0; p < PERIODS; ++p) {
                std::cout << std::left << std::setw(10) << grid[d][p];
            }
            std::cout << "\n";
        }
        std::cout << "---------------------------------------------------------------------------------\n";
    }
}
