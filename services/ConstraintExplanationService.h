#pragma once

#include <string>
#include <vector>
#include "../models/ConstraintViolation.h"

// Forward declarations
class Timetable;
class DataManager;

class ConstraintExplanationService {
public:
    // Analyze the current timetable and data manager state, returning violations.
    // For now it simply forwards any placement reject log messages from DataManager.
    std::vector<ConstraintViolation> explain(const Timetable& timetable, const DataManager& dm) const;
};
