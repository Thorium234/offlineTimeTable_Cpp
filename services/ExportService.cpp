#include "ExportService.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <QDateTime>

bool ExportService::exportToCSV(const std::string& filename, const Timetable& timetable, const DataManager& dm) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return false;
    }

    int numDays = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());

    file << "Timetable Report,Quality Score:," << timetable.score << "/1000\n\n";

    for (const auto& pair : timetable.schedules) {
        int classId = pair.first;
        const auto& grid = pair.second;

        file << "CLASS:," << dm.getClassName(classId) << "\n";

        file << "DAY,";
        for (int p = 0; p < numPeriods; ++p) {
            file << "P" << (p + 1) << " (" << dm.getPeriodTime(dm.periods[p].id) << "),";
        }
        file << "\n";

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

    std::cerr << "Exported timetable to CSV successfully: " << filename << "\n";
    return true;
}

bool ExportService::exportToHTML(const std::string& filename, const Timetable& timetable, const DataManager& dm) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return false;
    }

    int numDays = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");

    file << "<!DOCTYPE html>\n<html>\n<head>\n";
    file << "<meta charset='UTF-8'>\n";
    file << "<title>School Timetable Report</title>\n";
    file << "<style>\n";
    file << "@page { margin: 12mm 10mm; }\n";
    file << "* { box-sizing: border-box; }\n";
    file << "body { font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; font-size: 9pt; color: #1e293b; margin: 0; padding: 20px; -webkit-print-color-adjust: exact; print-color-adjust: exact; }\n";
    file << ".report-header { text-align: center; padding: 24px 0 16px; margin-bottom: 20px; border-bottom: 3px solid #f0a500; }\n";
    file << ".report-header h1 { font-size: 22pt; font-weight: 800; color: #1e3a5f; margin: 0 0 4px; }\n";
    file << ".report-header .meta { font-size: 9pt; color: #64748b; margin: 4px 0; }\n";
    file << ".score { font-size: 10pt; font-weight: 600; color: #475569; margin: 8px 0 0; }\n";
    file << ".entity-section { page-break-before: always; padding-top: 8px; }\n";
    file << ".entity-section:first-of-type { page-break-before: avoid; }\n";
    file << ".entity-title { font-size: 14pt; font-weight: 700; color: #1e3a5f; margin: 0 0 10px; padding-bottom: 6px; border-bottom: 2px solid #e2e8f0; }\n";
    file << "table { width: 100%; border-collapse: collapse; table-layout: fixed; }\n";
    file << "thead th { background: #1e3a5f; color: #fff; padding: 7px 4px; font-size: 8pt; font-weight: 600; text-align: center; border: 1px solid #1e3a5f; }\n";
    file << "thead th.day-header { width: 10%; }\n";
    file << "thead th.period-col { width: 8%; }\n";
    file << "tbody td { border: 1px solid #cbd5e1; padding: 5px 3px; vertical-align: middle; text-align: center; height: 48px; }\n";
    file << "tbody tr:nth-child(even) td { background: #f8fafc; }\n";
    file << "td.period-label { background: #f1f5f9; font-weight: 700; font-size: 8pt; color: #475569; width: 8%; }\n";
    file << "td.period-label small { display: block; font-weight: 400; color: #94a3b8; font-size: 6.5pt; }\n";
    file << ".slot-card { display: inline-block; border-radius: 4px; padding: 4px 6px; color: #fff; width: 100%; line-height: 1.4; }\n";
    file << ".slot-subj { font-weight: 700; font-size: 8.5pt; display: block; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }\n";
    file << ".slot-detail { font-size: 6.5pt; display: block; opacity: 0.9; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }\n";
    file << ".empty-cell { color: #cbd5e1; font-size: 9pt; }\n";
    file << ".unsched-section { page-break-before: always; }\n";
    file << ".unsched-title { font-size: 14pt; font-weight: 700; color: #dc2626; margin: 0 0 10px; padding-bottom: 6px; border-bottom: 2px solid #fecaca; }\n";
    file << ".unsched-table th { background: #dc2626; color: #fff; padding: 7px 8px; font-size: 8pt; font-weight: 600; text-align: left; border: 1px solid #dc2626; }\n";
    file << ".unsched-table td { border: 1px solid #fecaca; padding: 6px 8px; font-size: 8pt; }\n";
    file << ".unsched-table tr:nth-child(even) td { background: #fef2f2; }\n";
    file << "</style>\n</head>\n<body>\n";

    file << "<div class='report-header'>\n";
    file << "<h1>School Timetable Report</h1>\n";
    file << "<div class='meta'>Generated: <strong>" << now.toStdString() << "</strong></div>\n";
    file << "<div class='score'>Quality Score: " << timetable.score << " / 1000</div>\n";
    file << "</div>\n";

    for (const auto& pair : timetable.schedules) {
        int classId = pair.first;
        const auto& grid = pair.second;

        file << "<div class='entity-section'>\n";
        file << "<div class='entity-title'>Class: " << dm.getClassName(classId) << "</div>\n";
        file << "<table>\n<thead>\n<tr>\n";
        file << "<th class='period-col'>Period</th>\n";
        for (int d = 0; d < numDays; ++d) {
            file << "<th class='day-header'>" << dm.getDayName(dm.days[d].id) << "</th>\n";
        }
        file << "</tr>\n</thead>\n<tbody>\n";

        for (int p = 0; p < numPeriods; ++p) {
            file << "<tr>\n";
            file << "<td class='period-label'>P" << (p + 1)
                 << "<small>" << dm.getPeriodTime(dm.periods[p].id) << "</small></td>\n";
            for (int d = 0; d < numDays; ++d) {
                file << "<td>";
                bool found = false;
                if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                    const auto& cell = grid[d][p];
                    if (!cell.isEmpty()) {
                        std::string wtBadge;
                        if (cell.weekType == 1) wtBadge = " <span style='font-size:6pt;font-weight:700;background:rgba(255,255,255,0.3);border-radius:2px;padding:0 3px;'>A</span>";
                        else if (cell.weekType == 2) wtBadge = " <span style='font-size:6pt;font-weight:700;background:rgba(255,255,255,0.3);border-radius:2px;padding:0 3px;'>B</span>";
                        file << "<div class='slot-card' style='background:#4A90D9'>";
                        file << "<span class='slot-subj'>" << dm.getSubjectName(cell.subjectId) << wtBadge << "</span>";
                        file << "<span class='slot-detail'>" << dm.getTeacherName(cell.teacherId)
                             << " &middot; " << dm.getRoomName(cell.roomId) << "</span>";
                        file << "</div>";
                        found = true;
                    }
                }
                if (!found) {
                    file << "<span class='empty-cell'>&mdash;</span>";
                }
                file << "</td>\n";
            }
            file << "</tr>\n";
        }
        file << "</tbody>\n</table>\n</div>\n";
    }

    if (!timetable.unscheduledLessons.empty()) {
        file << "<div class='unsched-section'>\n";
        file << "<div class='unsched-title'>Unscheduled Lessons (" << timetable.unscheduledLessons.size() << ")</div>\n";
        file << "<table class='unsched-table'>\n<thead>\n<tr>\n";
        file << "<th style='width:25%'>Class</th>\n";
        file << "<th style='width:25%'>Subject</th>\n";
        file << "<th style='width:20%'>Teacher</th>\n";
        file << "<th style='width:12%'>Unscheduled Periods</th>\n";
        file << "<th style='width:18%'>Reason</th>\n";
        file << "</tr>\n</thead>\n<tbody>\n";
        for (const auto& ul : timetable.unscheduledLessons) {
            file << "<tr>\n";
            file << "<td><strong>" << dm.getClassName(ul.classId) << "</strong></td>\n";
            file << "<td>" << dm.getSubjectName(ul.subjectId) << "</td>\n";
            file << "<td>" << dm.getTeacherName(ul.teacherId) << "</td>\n";
            file << "<td>" << ul.periodsCount << "</td>\n";
            file << "<td style='color:#dc2626;'>" << ul.reason << "</td>\n";
            file << "</tr>\n";
        }
        file << "</tbody>\n</table>\n</div>\n";
    }

    file << "</body>\n</html>\n";
    std::cerr << "Exported timetable to HTML successfully: " << filename << "\n";
    return true;
}
