#pragma once

#include <string>
#include <optional>
#include "../timetable/Timetable.h"
#include "../services/AnalyticsService.h"

class PdfReportService {
public:
    bool generate(const Timetable& timetable,
                  const std::optional<AnalyticsReport>& analytics,
                  const std::string& outputPath);

    // Generate PDF to a file and return path (for file-based export)
    bool exportToPdf(const Timetable& timetable, const DataManager& dm,
                     const std::string& outputPath);
};
