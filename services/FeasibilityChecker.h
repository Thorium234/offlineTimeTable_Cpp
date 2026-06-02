#pragma once
#include "DataManager.h"
#include <vector>
#include <string>

struct FeasibilityError {
    std::string category;  // "SLOT_OVERFLOW", "ROOM_SHORTAGE", "TEACHER_OVERLOAD"
    std::string message;
};

class FeasibilityChecker {
public:
    std::vector<FeasibilityError> check(const DataManager& dm) const;

private:
    void checkSlotOverflow(const DataManager& dm, std::vector<FeasibilityError>& errors) const;
    void checkRoomShortage(const DataManager& dm, std::vector<FeasibilityError>& errors) const;
    void checkTeacherOverload(const DataManager& dm, std::vector<FeasibilityError>& errors) const;
    void checkConsecutiveBlocks(const DataManager& dm, std::vector<FeasibilityError>& errors) const;
    void checkSubjectDistribution(const DataManager& dm, std::vector<FeasibilityError>& errors) const;
};
