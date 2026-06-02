#pragma once
#include "DataManager.h"
#include "../timetable/Timetable.h"

class TimetableEngine {
public:
    Timetable generate(const DataManager& dm);
};
