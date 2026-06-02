#pragma once

#include <string>
#include "../timetable/Timetable.h"

class PdfReportService {
public:
    // Generates a PDF report for the given timetable.
    // If analytics is provided, a summary page is added.
    bool generate(const Timetable& timetable,
                 const std::optional<AnalyticsReport>& analytics,
                 const std::string& outputPath);
};
