#include "PdfReportService.h"
#include <QPrinter>
#include <QTextDocument>
#include <QPageLayout>
#include <QPageSize>
#include <QFont>
#include <QColor>
#include <QRectF>
#include <QDateTime>

bool PdfReportService::generate(const Timetable& /*timetable*/,
                                const std::optional<AnalyticsReport>& /*analytics*/,
                                const std::string& /*outputPath*/) {
    return false;
}

bool PdfReportService::exportToPdf(const Timetable& timetable, const DataManager& dm,
                                   const std::string& outputPath) {
    int numDays = static_cast<int>(dm.days.size());
    int numPeriods = static_cast<int>(dm.periods.size());

    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");

    QString html;
    html += "<!DOCTYPE html>\n<html>\n<head>\n";
    html += "<meta charset='UTF-8'>\n";
    html += "<title>School Timetable Report</title>\n";
    html += "<style>\n";
    html += "@page { margin: 12mm 10mm; }\n";
    html += "* { box-sizing: border-box; }\n";
    html += "body { font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; font-size: 9pt; color: #1e293b; margin: 0; padding: 0; -webkit-print-color-adjust: exact; print-color-adjust: exact; }\n";
    html += ".report-header { text-align: center; padding: 24px 0 16px; margin-bottom: 20px; border-bottom: 3px solid #f0a500; }\n";
    html += ".report-header h1 { font-size: 22pt; font-weight: 800; color: #1e3a5f; margin: 0 0 4px; }\n";
    html += ".report-header .meta { font-size: 9pt; color: #64748b; margin: 4px 0; }\n";
    html += ".score { font-size: 10pt; font-weight: 600; color: #475569; margin: 8px 0 0; }\n";
    html += ".entity-section { page-break-before: always; padding-top: 8px; }\n";
    html += ".entity-section:first-of-type { page-break-before: avoid; }\n";
    html += ".entity-title { font-size: 14pt; font-weight: 700; color: #1e3a5f; margin: 0 0 10px; padding-bottom: 6px; border-bottom: 2px solid #e2e8f0; }\n";
    html += "table { width: 100%; border-collapse: collapse; table-layout: fixed; }\n";
    html += "thead th { background: #1e3a5f; color: #fff; padding: 7px 4px; font-size: 8pt; font-weight: 600; text-align: center; border: 1px solid #1e3a5f; }\n";
    html += "thead th.day-header { width: 10%; }\n";
    html += "thead th.period-col { width: 8%; }\n";
    html += "tbody td { border: 1px solid #cbd5e1; padding: 5px 3px; vertical-align: middle; text-align: center; height: 48px; }\n";
    html += "tbody tr:nth-child(even) td { background: #f8fafc; }\n";
    html += "td.period-label { background: #f1f5f9; font-weight: 700; font-size: 8pt; color: #475569; width: 8%; }\n";
    html += "td.period-label small { display: block; font-weight: 400; color: #94a3b8; font-size: 6.5pt; }\n";
    html += ".slot-card { display: inline-block; border-radius: 4px; padding: 4px 6px; color: #fff; width: 100%; line-height: 1.4; }\n";
    html += ".slot-subj { font-weight: 700; font-size: 8.5pt; display: block; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }\n";
    html += ".slot-detail { font-size: 6.5pt; display: block; opacity: 0.9; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }\n";
    html += ".empty-cell { color: #cbd5e1; font-size: 9pt; }\n";
    html += ".unsched-section { page-break-before: always; }\n";
    html += ".unsched-title { font-size: 14pt; font-weight: 700; color: #dc2626; margin: 0 0 10px; padding-bottom: 6px; border-bottom: 2px solid #fecaca; }\n";
    html += ".unsched-table th { background: #dc2626; color: #fff; padding: 7px 8px; font-size: 8pt; font-weight: 600; text-align: left; border: 1px solid #dc2626; }\n";
    html += ".unsched-table td { border: 1px solid #fecaca; padding: 6px 8px; font-size: 8pt; }\n";
    html += ".unsched-table tr:nth-child(even) td { background: #fef2f2; }\n";
    html += "</style>\n</head>\n<body>\n";

    html += "<div class='report-header'>\n";
    html += "<h1>School Timetable Report</h1>\n";
    html += "<div class='meta'>Generated: <strong>" + now + "</strong></div>\n";
    html += "<div class='score'>Quality Score: " + QString::number(timetable.score) + " / 1000</div>\n";
    html += "</div>\n";

    for (const auto& pair : timetable.schedules) {
        int classId = pair.first;
        const auto& grid = pair.second;

        html += "<div class='entity-section'>\n";
        html += "<div class='entity-title'>Class: " + QString::fromStdString(dm.getClassName(classId)) + "</div>\n";
        html += "<table>\n<thead>\n<tr>\n";
        html += "<th class='period-col'>Period</th>\n";
        for (int d = 0; d < numDays; ++d) {
            html += "<th class='day-header'>" + QString::fromStdString(dm.getDayName(dm.days[d].id)) + "</th>\n";
        }
        html += "</tr>\n</thead>\n<tbody>\n";

        for (int p = 0; p < numPeriods; ++p) {
            html += "<tr>\n";
            html += "<td class='period-label'>P" + QString::number(p + 1) +
                    "<small>" + QString::fromStdString(dm.getPeriodTime(dm.periods[p].id)) + "</small></td>\n";
            for (int d = 0; d < numDays; ++d) {
                html += "<td>";
                bool found = false;
                if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                    const auto& cell = grid[d][p];
                    if (!cell.isEmpty()) {
                        QString col = "#4A90D9";
                        QString wtBadge;
                        if (cell.weekType == 1) wtBadge = " <span style='font-size:6pt;font-weight:700;background:rgba(255,255,255,0.3);border-radius:2px;padding:0 3px;'>A</span>";
                        else if (cell.weekType == 2) wtBadge = " <span style='font-size:6pt;font-weight:700;background:rgba(255,255,255,0.3);border-radius:2px;padding:0 3px;'>B</span>";
                        html += "<div class='slot-card' style='background:" + col + "'>";
                        html += "<span class='slot-subj'>" + QString::fromStdString(dm.getSubjectName(cell.subjectId)) + wtBadge + "</span>";
                        html += "<span class='slot-detail'>" + QString::fromStdString(dm.getTeacherName(cell.teacherId)) +
                                " &middot; " + QString::fromStdString(dm.getRoomName(cell.roomId)) + "</span>";
                        html += "</div>";
                        found = true;
                    }
                }
                if (!found) {
                    html += "<span class='empty-cell'>&mdash;</span>";
                }
                html += "</td>\n";
            }
            html += "</tr>\n";
        }
        html += "</tbody>\n</table>\n</div>\n";
    }

    if (!timetable.unscheduledLessons.empty()) {
        html += "<div class='unsched-section'>\n";
        html += "<div class='unsched-title'>Unscheduled Lessons (" + QString::number(timetable.unscheduledLessons.size()) + ")</div>\n";
        html += "<table class='unsched-table'>\n<thead>\n<tr>\n";
        html += "<th style='width:25%'>Class</th>\n";
        html += "<th style='width:25%'>Subject</th>\n";
        html += "<th style='width:20%'>Teacher</th>\n";
        html += "<th style='width:12%'>Unscheduled Periods</th>\n";
        html += "<th style='width:18%'>Reason</th>\n";
        html += "</tr>\n</thead>\n<tbody>\n";
        for (const auto& ul : timetable.unscheduledLessons) {
            html += "<tr>\n";
            html += "<td><strong>" + QString::fromStdString(dm.getClassName(ul.classId)) + "</strong></td>\n";
            html += "<td>" + QString::fromStdString(dm.getSubjectName(ul.subjectId)) + "</td>\n";
            html += "<td>" + QString::fromStdString(dm.getTeacherName(ul.teacherId)) + "</td>\n";
            html += "<td>" + QString::number(ul.periodsCount) + "</td>\n";
            html += "<td>" + QString::fromStdString(ul.reason) + "</td>\n";
            html += "</tr>\n";
        }
        html += "</tbody>\n</table>\n</div>\n";
    }

    html += "</body>\n</html>\n";

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(QString::fromStdString(outputPath));
    printer.setPageSize(QPageSize(QPageSize::A3));
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setHtml(html);
    doc.setPageSize(QSizeF(printer.pageRect(QPrinter::DevicePixel).size()));
    doc.print(&printer);

    return true;
}
