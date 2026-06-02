#pragma once
#include "../timetable/Timetable.h"
#include "DataManager.h"
#include <string>

class ExportService {
public:
    static bool exportToCSV(const std::string& filename, const Timetable& timetable, const DataManager& dm);
    static bool exportToHTML(const std::string& filename, const Timetable& timetable, const DataManager& dm);
};
