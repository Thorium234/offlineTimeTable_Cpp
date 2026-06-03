#pragma once

#include <string>
#include <vector>

// Forward declarations
class Timetable;
class DataManager;

struct ConstraintViolation {
    std::string type;          // e.g., "Teacher Overload"
    std::string explanation;   // Human readable description
    std::string recommendation; // Suggested action
};

class ConstraintExplanationService {
public:
    // Analyze the current timetable and data manager state, returning violations.
    // For now it simply forwards any placement reject log messages from DataManager.
    std::vector<ConstraintViolation> explain(const Timetable& timetable, const DataManager& dm) const;
};
