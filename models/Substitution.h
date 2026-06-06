#pragma once
#include <string>

enum class SubstitutionStatus {
    PENDING = 0,
    ASSIGNED = 1,
    COMPLETED = 2,
    CANCELLED = 3
};

inline std::string substitutionStatusToString(SubstitutionStatus s) {
    switch (s) {
        case SubstitutionStatus::PENDING:   return "PENDING";
        case SubstitutionStatus::ASSIGNED:  return "ASSIGNED";
        case SubstitutionStatus::COMPLETED: return "COMPLETED";
        case SubstitutionStatus::CANCELLED: return "CANCELLED";
        default: throw std::logic_error("Unknown substitution status");
    }
}

inline SubstitutionStatus substitutionStatusFromString(const std::string& s) {
    if (s == "PENDING")   return SubstitutionStatus::PENDING;
    if (s == "ASSIGNED")  return SubstitutionStatus::ASSIGNED;
    if (s == "COMPLETED") return SubstitutionStatus::COMPLETED;
    if (s == "CANCELLED") return SubstitutionStatus::CANCELLED;
    throw std::logic_error("Invalid substitution status: " + s);
}

struct Substitution {
    int id;
    int originalTeacherId;
    int substituteTeacherId = -1;
    int subjectId;
    int classId;
    int dayId;
    int periodId;
    SubstitutionStatus status = SubstitutionStatus::PENDING;
    std::string reason;
    std::string date; // ISO date string
};
