#pragma once
#include <string>

/**
 * Represents a single constraint violation detected after timetable generation.
 * It provides a human‑readable explanation and a recommendation for the
 * administrator to resolve the issue.
 */
struct ConstraintViolation {
    std::string type;           // e.g. "Teacher Overload", "Room Shortage"
    std::string explanation;    // Detailed description of the problem
    std::string recommendation; // Suggested corrective action
};
