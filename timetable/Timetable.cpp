#include "Timetable.h"
#include "../services/DataManager.h"
#include <iostream>
#include <iomanip>

void Timetable::initClass(int classId, int totalDays, int totalPeriods) {
    schedules[classId] = std::vector<std::vector<TimetableCell>>(totalDays, std::vector<TimetableCell>(totalPeriods));
}

void Timetable::setSlot(int classId, int dayIndex, int periodIndex, int subjectId, int teacherId, int roomId) {
    if (schedules.find(classId) == schedules.end()) {
        schedules[classId] = std::vector<std::vector<TimetableCell>>(5, std::vector<TimetableCell>(8));
    }
    schedules[classId][dayIndex][periodIndex] = TimetableCell{subjectId, teacherId, roomId};
}

void Timetable::clearSlot(int classId, int dayIndex, int periodIndex) {
    auto it = schedules.find(classId);
    if (it != schedules.end()) {
        if (dayIndex >= 0 && dayIndex < static_cast<int>(it->second.size())) {
            if (periodIndex >= 0 && periodIndex < static_cast<int>(it->second[dayIndex].size())) {
                it->second[dayIndex][periodIndex] = TimetableCell{};
            }
        }
    }
}

TimetableCell Timetable::getSlot(int classId, int dayIndex, int periodIndex) const {
    auto it = schedules.find(classId);
    if (it != schedules.end()) {
        if (dayIndex >= 0 && dayIndex < static_cast<int>(it->second.size())) {
            if (periodIndex >= 0 && periodIndex < static_cast<int>(it->second[dayIndex].size())) {
                return it->second[dayIndex][periodIndex];
            }
        }
    }
    return TimetableCell{};
}

void Timetable::print(const DataManager& dm) const {
    if (schedules.empty()) {
        std::cout << "\nNo timetable generated yet or no classes exist.\n";
        return;
    }

    int numDays = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());

    std::cout << "\n========================================================================================================================\n";
    std::cout << " GENERATED TIMETABLE (Score: " << score << " / 1000)\n";
    std::cout << "========================================================================================================================\n";

    for (const auto& pair : schedules) {
        int classId = pair.first;
        const auto& grid = pair.second;

        std::cout << "\n------------------------------------------------------------------------------------------------------------------------\n";
        std::cout << " CLASS: " << dm.getClassName(classId) << " (Students: " << dm.classes[classId - 1].studentCount << ")\n";
        std::cout << "------------------------------------------------------------------------------------------------------------------------\n";

        // Print header
        std::cout << std::left << std::setw(12) << "DAY";
        for (int p = 0; p < numPeriods; ++p) {
            std::string pHeader = "P" + std::to_string(p + 1) + " (" + dm.getPeriodTime(dm.periods[p].id) + ")";
            std::cout << std::left << std::setw(28) << pHeader;
        }
        std::cout << "\n------------------------------------------------------------------------------------------------------------------------\n";

        // Print days
        for (int d = 0; d < numDays; ++d) {
            std::cout << std::left << std::setw(12) << dm.getDayName(dm.days[d].id);
            for (int p = 0; p < numPeriods; ++p) {
                std::string cellText = "---";
                if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                    const auto& cell = grid[d][p];
                    if (!cell.isEmpty()) {
                        cellText = dm.getSubjectName(cell.subjectId) + " (" +
                                   dm.getTeacherName(cell.teacherId) + "/" +
                                   dm.getRoomName(cell.roomId) + ")";
                    }
                }
                std::cout << std::left << std::setw(28) << cellText;
            }
            std::cout << "\n";
        }
        std::cout << "------------------------------------------------------------------------------------------------------------------------\n";
    }

    if (!unscheduledLessons.empty()) {
        std::cout << "\n========================================================================================================================\n";
        std::cout << " UNSCHEDULED LESSONS REPORT\n";
        std::cout << "========================================================================================================================\n";
        std::cout << std::left << std::setw(20) << "CLASS"
                  << std::left << std::setw(20) << "SUBJECT"
                  << std::left << std::setw(20) << "TEACHER"
                  << std::left << std::setw(15) << "MISSING"
                  << "REASON\n";
        std::cout << "------------------------------------------------------------------------------------------------------------------------\n";
        for (const auto& ul : unscheduledLessons) {
            std::cout << std::left << std::setw(20) << dm.getClassName(ul.classId)
                      << std::left << std::setw(20) << dm.getSubjectName(ul.subjectId)
                      << std::left << std::setw(20) << dm.getTeacherName(ul.teacherId)
                      << std::left << std::setw(15) << ul.periodsCount
                      << ul.reason << "\n";
        }
        std::cout << "========================================================================================================================\n";
    }
}
