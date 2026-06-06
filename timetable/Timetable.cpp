#include "Timetable.h"
#include "../services/DataManager.h"
#include <iostream>
#include <iomanip>

void Timetable::initClass(int classId, int totalDays, int totalPeriods) {
    schedules[classId] = std::vector<std::vector<TimetableCell>>(totalDays, std::vector<TimetableCell>(totalPeriods));
}

void Timetable::setSlot(int classId, int dayIndex, int periodIndex, int subjectId, int teacherId, int roomId, int weekType) {
    if (schedules.find(classId) == schedules.end()) {
        schedules[classId] = std::vector<std::vector<TimetableCell>>(5, std::vector<TimetableCell>(8));
    }
    schedules[classId][dayIndex][periodIndex] = TimetableCell{subjectId, teacherId, roomId, false, weekType};
}

void Timetable::setSlotLocked(int classId, int dayIndex, int periodIndex, int subjectId, int teacherId, int roomId, bool locked) {
    if (schedules.find(classId) == schedules.end()) {
        schedules[classId] = std::vector<std::vector<TimetableCell>>(5, std::vector<TimetableCell>(8));
    }
    if (dayIndex >= 0 && dayIndex < static_cast<int>(schedules[classId].size()) &&
        periodIndex >= 0 && periodIndex < static_cast<int>(schedules[classId][dayIndex].size())) {
        schedules[classId][dayIndex][periodIndex] = TimetableCell{subjectId, teacherId, roomId, locked};
    }
}

bool Timetable::moveSlot(int srcClassId, int srcDay, int srcPeriod,
                          int dstClassId, int dstDay, int dstPeriod) {
    auto srcIt = schedules.find(srcClassId);
    auto dstIt = schedules.find(dstClassId);
    if (srcIt == schedules.end() || dstIt == schedules.end()) return false;

    auto &srcGrid = srcIt->second;
    auto &dstGrid = dstIt->second;

    if (srcDay < 0 || srcDay >= static_cast<int>(srcGrid.size()) ||
        srcPeriod < 0 || srcPeriod >= static_cast<int>(srcGrid[srcDay].size()) ||
        dstDay < 0 || dstDay >= static_cast<int>(dstGrid.size()) ||
        dstPeriod < 0 || dstPeriod >= static_cast<int>(dstGrid[dstDay].size())) {
        return false;
    }

    TimetableCell &src = srcGrid[srcDay][srcPeriod];
    TimetableCell &dst = dstGrid[dstDay][dstPeriod];

    if (src.locked || dst.locked) return false;
    if (src.isEmpty()) return false;

    dst = src;
    src = TimetableCell{};
    return true;
}

bool Timetable::swapSlots(int classA, int dayA, int periodA,
                           int classB, int dayB, int periodB) {
    auto itA = schedules.find(classA);
    auto itB = schedules.find(classB);
    if (itA == schedules.end() || itB == schedules.end()) return false;

    auto &gridA = itA->second;
    auto &gridB = itB->second;

    if (dayA < 0 || dayA >= static_cast<int>(gridA.size()) ||
        periodA < 0 || periodA >= static_cast<int>(gridA[dayA].size()) ||
        dayB < 0 || dayB >= static_cast<int>(gridB.size()) ||
        periodB < 0 || periodB >= static_cast<int>(gridB[dayB].size())) {
        return false;
    }

    TimetableCell &cellA = gridA[dayA][periodA];
    TimetableCell &cellB = gridB[dayB][periodB];

    if (cellA.locked || cellB.locked) return false;

    std::swap(cellA, cellB);
    return true;
}

bool Timetable::isSlotLocked(int classId, int dayIndex, int periodIndex) const {
    auto it = schedules.find(classId);
    if (it == schedules.end()) return false;
    const auto &grid = it->second;
    if (dayIndex < 0 || dayIndex >= static_cast<int>(grid.size())) return false;
    if (periodIndex < 0 || periodIndex >= static_cast<int>(grid[dayIndex].size())) return false;
    return grid[dayIndex][periodIndex].locked;
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
        std::cout << " CLASS: " << dm.getClassName(classId) << " (Students: ";
        {
            bool found = false;
            for (const auto &c : dm.classes) {
                if (c.id == classId) { std::cout << c.studentCount; found = true; break; }
            }
            if (!found) std::cout << "?";
        }
        std::cout << ")\n";
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
