#pragma once
#include <string>
#include <stdexcept>

enum class PreferenceType {
    PREFERRED,
    NEUTRAL,
    UNDESIRABLE
};

inline std::string preferenceTypeToString(PreferenceType p) {
    switch (p) {
        case PreferenceType::PREFERRED:   return "PREFERRED";
        case PreferenceType::NEUTRAL:     return "NEUTRAL";
        case PreferenceType::UNDESIRABLE: return "UNDESIRABLE";
        default:
            throw std::logic_error("Unknown PreferenceType");
    }
}

struct TeacherPreference {
    int teacherId;
    int dayId;
    int periodId;
    PreferenceType type;
};
