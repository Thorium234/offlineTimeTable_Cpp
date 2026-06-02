#include "FeasibilityChecker.h"
#include <map>
#include <iostream>

std::vector<FeasibilityError> FeasibilityChecker::check(const DataManager& dm) const {
    std::vector<FeasibilityError> errors;
    checkSlotOverflow(dm, errors);
    checkRoomShortage(dm, errors);
    checkTeacherOverload(dm, errors);
    checkConsecutiveBlocks(dm, errors);
    checkSubjectDistribution(dm, errors);
    return errors;
}

void FeasibilityChecker::checkSlotOverflow(const DataManager& dm, std::vector<FeasibilityError>& errors) const {
    int totalSlots = static_cast<int>(dm.days.size() * dm.periods.size());

    // Per-class: sum of periodsPerWeek must not exceed total slots
    std::map<int, int> classLoad;
    for (const auto& lesson : dm.lessons) {
        classLoad[lesson.classId] += lesson.periodsPerWeek;
    }

    for (const auto& pair : classLoad) {
        if (pair.second > totalSlots) {
            std::string className = dm.getClassName(pair.first);
            errors.push_back(FeasibilityError{
                "SLOT_OVERFLOW",
                "Class '" + className + "' requires " + std::to_string(pair.second) +
                " periods but only " + std::to_string(totalSlots) + " slots are available."
            });
        }
    }
}

void FeasibilityChecker::checkRoomShortage(const DataManager& dm, std::vector<FeasibilityError>& errors) const {
    int totalSlots = static_cast<int>(dm.days.size() * dm.periods.size());

    // For each room type, compute:
    //   demand = total periods requiring that room type
    //   supply = number of rooms of that type × total slots
    std::map<int, int> roomTypeDemand;
    for (const auto& lesson : dm.lessons) {
        int reqType = dm.getSubjectRequiredRoomTypeId(lesson.subjectId);
        roomTypeDemand[reqType] += lesson.periodsPerWeek;
    }

    std::map<int, int> roomTypeSupply;
    for (const auto& room : dm.rooms) {
        roomTypeSupply[room.roomTypeId] += totalSlots;
    }

    for (const auto& pair : roomTypeDemand) {
        int supply = roomTypeSupply[pair.first];
        if (pair.second > supply) {
            std::string typeName = dm.getRoomTypeName(pair.first);
            int roomCount = 0;
            for (const auto& room : dm.rooms) {
                if (room.roomTypeId == pair.first) roomCount++;
            }
            errors.push_back(FeasibilityError{
                "ROOM_SHORTAGE",
                "Room type '" + typeName + "' has " + std::to_string(roomCount) +
                " room(s) providing " + std::to_string(supply) + " total slots, but " +
                std::to_string(pair.second) + " lesson-periods require it."
            });
        }
    }
}

void FeasibilityChecker::checkTeacherOverload(const DataManager& dm, std::vector<FeasibilityError>& errors) const {
    int totalSlots = static_cast<int>(dm.days.size() * dm.periods.size());

    // For each teacher: available slots = total - unavailability constraints
    // assigned periods = sum of periodsPerWeek
    std::map<int, int> teacherAssigned;
    for (const auto& lesson : dm.lessons) {
        teacherAssigned[lesson.teacherId] += lesson.periodsPerWeek;
    }

    for (const auto& pair : teacherAssigned) {
        // Count unavailability constraints for this teacher
        int unavailCount = 0;
        for (const auto& tc : dm.teacherConstraints) {
            if (tc.teacherId == pair.first) {
                unavailCount++;
            }
        }

        int availableSlots = totalSlots - unavailCount;
        if (pair.second > availableSlots) {
            std::string teacherName = dm.getTeacherName(pair.first);
            errors.push_back(FeasibilityError{
                "TEACHER_OVERLOAD",
                "Teacher '" + teacherName + "' is assigned " + std::to_string(pair.second) +
                " periods but only has " + std::to_string(availableSlots) +
                " available slots (" + std::to_string(totalSlots) + " total - " +
                std::to_string(unavailCount) + " unavailable)."
            });
        }
    }
}

void FeasibilityChecker::checkConsecutiveBlocks(const DataManager& dm, std::vector<FeasibilityError>& errors) const {
    int numPeriods = static_cast<int>(dm.periods.size());
    for (const auto& lesson : dm.lessons) {
        if (lesson.blockSize > numPeriods) {
            std::string className = dm.getClassName(lesson.classId);
            std::string subjectName = dm.getSubjectName(lesson.subjectId);
            errors.push_back(FeasibilityError{
                "CONSECUTIVE_BLOCK_OVERFLOW",
                "Lesson '" + subjectName + "' for class '" + className + 
                "' requires a block size of " + std::to_string(lesson.blockSize) + 
                " which exceeds the " + std::to_string(numPeriods) + " available periods per day."
            });
        }
    }
}

void FeasibilityChecker::checkSubjectDistribution(const DataManager& dm, std::vector<FeasibilityError>& errors) const {
    int numDays = static_cast<int>(dm.days.size());
    for (const auto& lesson : dm.lessons) {
        if (lesson.maxPerDay > 0) {
            int maxPossiblePeriods = lesson.maxPerDay * numDays;
            if (lesson.periodsPerWeek > maxPossiblePeriods) {
                std::string className = dm.getClassName(lesson.classId);
                std::string subjectName = dm.getSubjectName(lesson.subjectId);
                errors.push_back(FeasibilityError{
                    "SUBJECT_DISTRIBUTION_IMPOSSIBLE",
                    "Lesson '" + subjectName + "' for class '" + className + 
                    "' has " + std::to_string(lesson.periodsPerWeek) + " periods per week but a max limit of " + 
                    std::to_string(lesson.maxPerDay) + " per day over " + std::to_string(numDays) + " days " +
                    "(max possible " + std::to_string(maxPossiblePeriods) + ")."
                });
            }
        }
    }
}
