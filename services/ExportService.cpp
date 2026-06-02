#include "ExportService.h"
#include <iostream>
#include <fstream>
#include <iomanip>

bool ExportService::exportToCSV(const std::string& filename, const Timetable& timetable, const DataManager& dm) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error: Could not open file " << filename << " for writing.\n";
        return false;
    }

    int numDays = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());

    for (const auto& pair : timetable.schedules) {
        int classId = pair.first;
        const auto& grid = pair.second;

        file << "CLASS:," << dm.getClassName(classId) << "\n";
        
        // Header row
        file << "DAY,";
        for (int p = 0; p < numPeriods; ++p) {
            file << "P" << (p + 1) << " (" << dm.getPeriodTime(dm.periods[p].id) << "),";
        }
        file << "\n";

        // Days
        for (int d = 0; d < numDays; ++d) {
            file << dm.getDayName(dm.days[d].id) << ",";
            for (int p = 0; p < numPeriods; ++p) {
                std::string cellText = "---";
                if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                    const auto& cell = grid[d][p];
                    if (!cell.isEmpty()) {
                        cellText = dm.getSubjectName(cell.subjectId) + " (" +
                                   dm.getTeacherName(cell.teacherId) + "/" +
                                   dm.getRoomName(cell.roomId) + ")";
                    }
                }
                // escape quotes and commas
                file << "\"" << cellText << "\",";
            }
            file << "\n";
        }
        file << "\n\n";
    }

    if (!timetable.unscheduledLessons.empty()) {
        file << "UNSCHEDULED LESSONS REPORT\n";
        file << "CLASS,SUBJECT,TEACHER,MISSING PERIODS,REASON\n";
        for (const auto& ul : timetable.unscheduledLessons) {
            file << "\"" << dm.getClassName(ul.classId) << "\","
                 << "\"" << dm.getSubjectName(ul.subjectId) << "\","
                 << "\"" << dm.getTeacherName(ul.teacherId) << "\","
                 << ul.periodsCount << ","
                 << "\"" << ul.reason << "\"\n";
        }
    }

    std::cout << "Exported timetable to CSV successfully: " << filename << "\n";
    return true;
}

bool ExportService::exportToHTML(const std::string& filename, const Timetable& timetable, const DataManager& dm) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error: Could not open file " << filename << " for writing.\n";
        return false;
    }

    int numDays = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());

    file << "<!DOCTYPE html>\n<html>\n<head>\n";
    file << "<meta charset='UTF-8'>\n";
    file << "<title>School Timetable Report</title>\n";
    file << "<style>\n";
    file << "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f8fafc; color: #1e293b; margin: 0; padding: 40px; }\n";
    file << "h1 { text-align: center; color: #0f172a; margin-bottom: 20px; font-weight: 800; font-size: 2.25rem; }\n";
    file << ".score-banner { text-align: center; font-size: 1.1rem; color: #475569; margin-bottom: 40px; font-weight: 600; background: #f1f5f9; padding: 10px; border-radius: 8px; display: inline-block; left: 50%; transform: translateX(-50%); position: relative; }\n";
    file << ".container { max-width: 1400px; margin: 0 auto; }\n";
    file << ".card { background: white; border-radius: 16px; box-shadow: 0 10px 15px -3px rgba(0,0,0,0.05), 0 4px 6px -4px rgba(0,0,0,0.05); padding: 28px; margin-bottom: 45px; border: 1px solid #e2e8f0; }\n";
    file << ".card-title { font-size: 1.6rem; font-weight: 700; color: #2563eb; margin-top: 0; margin-bottom: 22px; border-bottom: 2px solid #eff6ff; padding-bottom: 12px; }\n";
    file << "table { width: 100%; border-collapse: collapse; text-align: left; table-layout: fixed; }\n";
    file << "th { background-color: #f1f5f9; color: #475569; font-weight: 600; padding: 14px; font-size: 0.9rem; border: 1px solid #e2e8f0; }\n";
    file << "td { padding: 14px; font-size: 0.85rem; border: 1px solid #e2e8f0; vertical-align: top; word-wrap: break-word; }\n";
    file << "tr:nth-child(even) td { background-color: #f8fafc; }\n";
    file << ".cell-empty { color: #94a3b8; font-style: italic; }\n";
    file << ".cell-subject { font-weight: 700; color: #0f172a; display: block; margin-bottom: 4px; }\n";
    file << ".cell-details { font-size: 0.75rem; color: #64748b; display: block; line-height: 1.4; }\n";
    file << "</style>\n</head>\n<body>\n<div class='container'>\n";
    file << "<h1>School Timetable Report</h1>\n";
    file << "<div class='score-banner'>Timetable Quality Score: " << timetable.score << " / 1000</div>\n<br>\n";

    for (const auto& pair : timetable.schedules) {
        int classId = pair.first;
        const auto& grid = pair.second;

        file << "<div class='card'>\n";
        file << "<div class='card-title'>Class: " << dm.getClassName(classId) << "</div>\n";
        file << "<table>\n<thead>\n<tr>\n";
        file << "<th style='width: 120px;'>Day</th>\n";
        for (int p = 0; p < numPeriods; ++p) {
            file << "<th>P" << (p + 1) << "<br><span style='font-size: 0.75rem; font-weight: normal; color: #64748b;'>" 
                 << dm.getPeriodTime(dm.periods[p].id) << "</span></th>\n";
        }
        file << "</tr>\n</thead>\n<tbody>\n";

        for (int d = 0; d < numDays; ++d) {
            file << "<tr>\n";
            file << "<td><strong>" << dm.getDayName(dm.days[d].id) << "</strong></td>\n";
            for (int p = 0; p < numPeriods; ++p) {
                file << "<td>";
                bool found = false;
                if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                    const auto& cell = grid[d][p];
                    if (!cell.isEmpty()) {
                        file << "<span class='cell-subject'>" << dm.getSubjectName(cell.subjectId) << "</span>";
                        file << "<span class='cell-details'><strong>Teacher:</strong> " << dm.getTeacherName(cell.teacherId) 
                             << "<br><strong>Room:</strong> " << dm.getRoomName(cell.roomId) << "</span>";
                        found = true;
                    }
                }
                if (!found) {
                    file << "<span class='cell-empty'>---</span>";
                }
                file << "</td>\n";
            }
            file << "</tr>\n";
        }
        file << "</tbody>\n</table>\n</div>\n";
    }

    if (!timetable.unscheduledLessons.empty()) {
        file << "<div class='card' style='border: 1px solid #fca5a5; background-color: #fef2f2;'>\n";
        file << "<div class='card-title' style='color: #dc2626; border-bottom: 2px solid #fee2e2;'>Unscheduled Lessons Report</div>\n";
        file << "<table>\n<thead>\n<tr>\n";
        file << "<th>Class</th>\n";
        file << "<th>Subject</th>\n";
        file << "<th>Teacher</th>\n";
        file << "<th>Missing Periods</th>\n";
        file << "<th>Reason</th>\n";
        file << "</tr>\n</thead>\n<tbody>\n";
        for (const auto& ul : timetable.unscheduledLessons) {
            file << "<tr>\n";
            file << "<td><strong>" << dm.getClassName(ul.classId) << "</strong></td>\n";
            file << "<td>" << dm.getSubjectName(ul.subjectId) << "</td>\n";
            file << "<td>" << dm.getTeacherName(ul.teacherId) << "</td>\n";
            file << "<td>" << ul.periodsCount << "</td>\n";
            file << "<td style='color: #dc2626;'>" << ul.reason << "</td>\n";
            file << "</tr>\n";
        }
        file << "</tbody>\n</table>\n</div>\n";
    }

    file << "</div>\n</body>\n</html>\n";
    std::cout << "Exported timetable to HTML successfully: " << filename << "\n";
    return true;
}
