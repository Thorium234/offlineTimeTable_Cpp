#include "Menu.h"
#include "../services/ExportService.h"
#include "../services/Benchmark.h"
#include "../services/AnalyticsService.h"
#include "../services/PdfReportService.h"
#include "../services/GeneticSolver.h"
#include <iostream>
#include <iomanip>
#include <string>

std::string Menu::readLineInput(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

int Menu::readIntInput(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        try {
            return std::stoi(line);
        } catch (...) {
            std::cout << "Invalid input. Please enter a number.\n";
        }
    }
}

void Menu::run() {
    while (true) {
        std::cout << "\n╔════════════════════════════════╗\n";
        std::cout << "║    TIMETABLE GENERATOR v2.0    ║\n";
        std::cout << "╠════════════════════════════════╣\n";
        std::cout << "║  1. Add Teacher                ║\n";
        std::cout << "║  2. Add Subject                ║\n";
        std::cout << "║  3. Add Class                  ║\n";
        std::cout << "║  4. Add Room                   ║\n";
        std::cout << "║  5. Add Lesson                 ║\n";
        std::cout << "║  6. Add Day                    ║\n";
        std::cout << "║  7. Add Period                 ║\n";
        std::cout << "║  8. Teacher Unavailability      ║\n";
        std::cout << "║  9. Add Room Type              ║\n";
        std::cout << "║ 10. Subject Room Requirement   ║\n";
        std::cout << "║ 11. Generate Timetable         ║\n";
        std::cout << "║ 12. View Timetable             ║\n";
        std::cout << "║ 13. Export (CSV/HTML)          ║\n";
        std::cout << "║ 14. Run Benchmark Suite        ║\n";
        std::cout << "║ 15. Add Fixed Event            ║\n";
        std::cout << "║ 16. Add Teacher Preference     ║\n";
        std::cout << "║ 17. View Teacher Timetable      ║\n";
        std::cout << "║ 18. View Room Timetable         ║\n";
        std::cout << "║ 19. View Analytics Dashboard    ║\n";
        std::cout << "║ 20. Export PDF Report           ║\n";
        std::cout << "║ 21. Solve with Genetic Algorithm║\n";
        std::cout << "║ 22. Exit                       ║\n";
        std::cout << "╚════════════════════════════════╝\n";
        std::cout << "║    TIMETABLE GENERATOR v2.0    ║\n";
        std::cout << "╠════════════════════════════════╣\n";
        std::cout << "║  1. Add Teacher                ║\n";
        std::cout << "║  2. Add Subject                ║\n";
        std::cout << "║  3. Add Class                  ║\n";
        std::cout << "║  4. Add Room                   ║\n";
        std::cout << "║  5. Add Lesson                 ║\n";
        std::cout << "║  6. Add Day                    ║\n";
        std::cout << "║  7. Add Period                 ║\n";
        std::cout << "║  8. Teacher Unavailability      ║\n";
        std::cout << "║ 9. Add Room Type              ║\n";
        std::cout << "║ 10. Subject Room Requirement   ║\n";
        std::cout << "║ 11. Generate Timetable         ║\n";
        std::cout << "║ 12. View Timetable             ║\n";
        std::cout << "║ 13. Export (CSV/HTML)           ║\n";
        std::cout << "║ 14. Run Benchmark Suite        ║\n";
        std::cout << "║ 15. Add Fixed Event            ║\n";
        std::cout << "║ 16. Add Teacher Preference     ║\n";
        std::cout << "║ 17. View Teacher Timetable      ║\n";
        std::cout << "║ 18. View Room Timetable         ║\n";
        std::cout << "║ 19. View Analytics Dashboard    ║\n";
        std::cout << "║ 20. Exit                       ║\n";
        std::cout << "╚════════════════════════════════╝\n";
        
        int choice = readIntInput("Select option: > ");
        
        switch (choice) {
            case 1: handleAddTeacher(); break;
            case 2: handleAddSubject(); break;
            case 3: handleAddClass(); break;
            case 4: handleAddRoom(); break;
            case 5: handleAddLesson(); break;
            case 6: handleAddDay(); break;
            case 7: handleAddPeriod(); break;
            case 8: handleAddTeacherConstraint(); break;
            case 9: handleAddRoomType(); break;
            case 10: handleSetSubjectRequirement(); break;
            case 11: handleGenerateTimetable(); break;
            case 12: handleViewTimetable(); break;
            case 13: handleExportTimetable(); break;
            case 14: handleRunBenchmark(); break;
            case 15: handleAddFixedEvent(); break;
            case 16: handleAddTeacherPreference(); break;
            case 17: handleViewTeacherTimetable(); break;
            case 18: handleViewRoomTimetable(); break;
            case 19: handleViewAnalytics(); break;
            case 20:
                std::cout << "\nExiting Timetable Generator. Goodbye!\n";
                return;
            default:
                std::cout << "Invalid option. Please choose between 1 and 20.\n";
        }
    }
}

void Menu::handleAddTeacher() {
    std::cout << "\n--- Add Teacher ---\n";
    std::string name = readLineInput("Enter Teacher Name:\n> ");
    if (name.empty()) {
        std::cout << "Teacher name cannot be empty.\n";
        return;
    }
    int id = dm.addTeacher(name);
    std::cout << "Teacher \"" << name << "\" added with ID " << id << ".\n";
}

void Menu::handleAddSubject() {
    std::cout << "\n--- Add Subject ---\n";
    std::string name = readLineInput("Enter Subject Name:\n> ");
    if (name.empty()) {
        std::cout << "Subject name cannot be empty.\n";
        return;
    }
    int id = dm.addSubject(name);
    std::cout << "Subject \"" << name << "\" added with ID " << id << ".\n";
}

void Menu::handleAddClass() {
    std::cout << "\n--- Add Class ---\n";
    std::string name = readLineInput("Enter Class Name:\n> ");
    if (name.empty()) {
        std::cout << "Class name cannot be empty.\n";
        return;
    }
    int studentCount = readIntInput("Enter Student Count:\n> ");
    if (studentCount <= 0) {
        std::cout << "Student count must be positive.\n";
        return;
    }
    int id = dm.addClass(name, studentCount);
    std::cout << "Class \"" << name << "\" (Students: " << studentCount << ") added with ID " << id << ".\n";
}

void Menu::handleAddRoom() {
    std::cout << "\n--- Add Room ---\n";
    std::string name = readLineInput("Enter Room Name:\n> ");
    if (name.empty()) {
        std::cout << "Room name cannot be empty.\n";
        return;
    }
    int capacity = readIntInput("Enter Room Capacity:\n> ");
    if (capacity <= 0) {
        std::cout << "Capacity must be positive.\n";
        return;
    }

    std::cout << "\nAvailable Room Types:\n";
    for (const auto& rt : dm.roomTypes) {
        std::cout << rt.id << ". " << rt.name << "\n";
    }
    int roomTypeId = readIntInput("Select Room Type ID:\n> ");
    if (!dm.roomTypeExists(roomTypeId)) {
        std::cout << "Invalid Room Type ID. Defaulting to Classroom (1).\n";
        roomTypeId = 1;
    }

    int id = dm.addRoom(name, capacity, roomTypeId);
    std::cout << "Room \"" << name << "\" (Capacity: " << capacity 
              << ", Type: " << dm.getRoomTypeName(roomTypeId) << ") added with ID " << id << ".\n";
}

void Menu::handleAddLesson() {
    std::cout << "\n--- Add Lesson ---\n";
    
    if (dm.teachers.empty() || dm.subjects.empty() || dm.classes.empty()) {
        std::cout << "Cannot add lesson: Please ensure teachers, subjects, and classes are registered first.\n";
        return;
    }

    std::cout << "\nTeachers\n";
    for (const auto& t : dm.teachers) {
        std::cout << t.id << ". " << t.name << "\n";
    }
    int teacherId = readIntInput("Teacher ID:\n> ");
    if (!dm.teacherExists(teacherId)) {
        std::cout << "Invalid Teacher ID.\n";
        return;
    }

    std::cout << "\nSubjects\n";
    for (const auto& s : dm.subjects) {
        std::cout << s.id << ". " << s.name << "\n";
    }
    int subjectId = readIntInput("Subject ID:\n> ");
    if (!dm.subjectExists(subjectId)) {
        std::cout << "Invalid Subject ID.\n";
        return;
    }

    std::cout << "\nClasses\n";
    for (const auto& c : dm.classes) {
        std::cout << c.id << ". " << c.name << " (Students: " << c.studentCount << ")\n";
    }
    int classId = readIntInput("Class ID:\n> ");
    if (!dm.classExists(classId)) {
        std::cout << "Invalid Class ID.\n";
        return;
    }

    int periods = readIntInput("Periods Per Week:\n> ");
    int maxPeriods = static_cast<int>(dm.days.size() * dm.periods.size());
    if (periods <= 0 || periods > maxPeriods) {
        std::cout << "Periods per week must be between 1 and " << maxPeriods << ".\n";
        return;
    }

    // Sprint 4: Block size (consecutive periods per session)
    int blockSize = 1;
    if (periods >= 2) {
        blockSize = readIntInput("Enter block size (e.g. 1 for single, 2 for double periods):\n> ");
        if (blockSize < 1) blockSize = 1;
        if (blockSize > periods) blockSize = periods;
    }

    // Sprint 3: Max per day
    int maxPerDay = readIntInput("Max periods per day for this subject (0 = no limit):\n> ");
    if (maxPerDay < 0) maxPerDay = 0;

    if (dm.addLesson(teacherId, subjectId, classId, periods, blockSize, maxPerDay)) {
        std::cout << "Lesson created successfully.";
        if (blockSize > 1) std::cout << " [Block size: " << blockSize << "]";
        if (maxPerDay > 0) std::cout << " [Max " << maxPerDay << " per day]";
        std::cout << "\n";
    } else {
        std::cout << "Failed to create lesson.\n";
    }
}

void Menu::handleAddDay() {
    std::cout << "\n--- Add Day ---\n";
    std::string name = readLineInput("Enter Day Name:\n> ");
    if (name.empty()) {
        std::cout << "Day name cannot be empty.\n";
        return;
    }
    int id = dm.addDay(name);
    std::cout << "Day \"" << name << "\" added with ID " << id << ".\n";
}

void Menu::handleAddPeriod() {
    std::cout << "\n--- Add Period ---\n";
    std::string start = readLineInput("Enter Start Time (HH:MM):\n> ");
    std::string end = readLineInput("Enter End Time (HH:MM):\n> ");
    if (start.empty() || end.empty()) {
        std::cout << "Start and End times cannot be empty.\n";
        return;
    }
    int id = dm.addPeriod(start, end);
    std::cout << "Period " << id << " (" << start << "-" << end << ") added.\n";
}

void Menu::handleAddTeacherConstraint() {
    std::cout << "\n--- Add Teacher Unavailability Constraint ---\n";
    if (dm.teachers.empty() || dm.days.empty() || dm.periods.empty()) {
        std::cout << "Cannot add constraint: Please register teachers, days, and periods first.\n";
        return;
    }

    // 1. Select Teacher
    std::cout << "\nTeachers\n";
    for (const auto& t : dm.teachers) {
        std::cout << t.id << ". " << t.name << "\n";
    }
    int teacherId = readIntInput("Select Teacher ID:\n> ");
    if (!dm.teacherExists(teacherId)) {
        std::cout << "Invalid Teacher ID.\n";
        return;
    }

    // 2. Select Day
    std::cout << "\nDays\n";
    for (const auto& d : dm.days) {
        std::cout << d.id << ". " << d.name << "\n";
    }
    int dayId = readIntInput("Select Day ID:\n> ");
    if (!dm.dayExists(dayId)) {
        std::cout << "Invalid Day ID.\n";
        return;
    }

    // 3. Select Period
    std::cout << "\nPeriods\n";
    for (const auto& p : dm.periods) {
        std::cout << p.id << ". P" << p.id << " (" << p.startTime << "-" << p.endTime << ")\n";
    }
    int periodId = readIntInput("Select Period ID:\n> ");
    if (!dm.periodExists(periodId)) {
        std::cout << "Invalid Period ID.\n";
        return;
    }

    if (dm.addTeacherConstraint(teacherId, dayId, periodId)) {
        std::cout << "Unavailability constraint recorded: Teacher " << dm.getTeacherName(teacherId)
                  << " is unavailable on " << dm.getDayName(dayId) << " during Period P" << periodId << ".\n";
    } else {
        std::cout << "Failed to record constraint.\n";
    }
}

void Menu::handleAddRoomType() {
    std::cout << "\n--- Add Room Type ---\n";
    std::string name = readLineInput("Enter Room Type Name (e.g., Drawing Lab):\n> ");
    if (name.empty()) {
        std::cout << "Room Type name cannot be empty.\n";
        return;
    }
    int id = dm.addRoomType(name);
    std::cout << "Room Type \"" << name << "\" added with ID " << id << ".\n";
}

void Menu::handleSetSubjectRequirement() {
    std::cout << "\n--- Set Subject Room Requirement ---\n";
    if (dm.subjects.empty()) {
        std::cout << "No subjects registered yet.\n";
        return;
    }

    // 1. Select Subject
    std::cout << "\nSubjects:\n";
    for (const auto& s : dm.subjects) {
        std::cout << s.id << ". " << s.name << "\n";
    }
    int subjectId = readIntInput("Select Subject ID:\n> ");
    if (!dm.subjectExists(subjectId)) {
        std::cout << "Invalid Subject ID.\n";
        return;
    }

    // 2. Select Room Type
    std::cout << "\nAvailable Room Types:\n";
    for (const auto& rt : dm.roomTypes) {
        std::cout << rt.id << ". " << rt.name << "\n";
    }
    int roomTypeId = readIntInput("Select Required Room Type ID:\n> ");
    if (!dm.roomTypeExists(roomTypeId)) {
        std::cout << "Invalid Room Type ID.\n";
        return;
    }

    if (dm.setSubjectRequirement(subjectId, roomTypeId)) {
        std::cout << "Subject Room Requirement saved successfully! Subject '" << dm.getSubjectName(subjectId)
                  << "' now requires room type: '" << dm.getRoomTypeName(roomTypeId) << "'.\n";
    } else {
        std::cout << "Failed to save requirement.\n";
    }
}

void Menu::handleGenerateTimetable() {
    std::cout << "\nGenerating Timetable...\n";
    if (dm.lessons.empty()) {
        std::cout << "Cannot generate: No lessons have been assigned.\n";
        return;
    }
    timetable = engine.generate(dm);
    timetableGenerated = true;
    std::cout << "Timetable generated successfully! (Quality Score: " << timetable.score << "/1000)\n";
    if (!timetable.unscheduledLessons.empty()) {
        std::cout << "[Warning] There are " << timetable.unscheduledLessons.size() << " unscheduled lesson requirements.\n";
        std::cout << "Please select option 12 (View Timetable) to see the detailed report.\n";
    }
}

void Menu::handleViewTimetable() {
    if (!timetableGenerated) {
        std::cout << "\nPlease generate the timetable first (Option 11).\n";
        return;
    }
    timetable.print(dm);
}

void Menu::handleExportTimetable() {
    if (!timetableGenerated) {
        std::cout << "\nPlease generate the timetable first before exporting (Option 11).\n";
        return;
    }

    std::cout << "\n--- Export Timetable ---\n";
    std::cout << "1. Export to CSV\n";
    std::cout << "2. Export to HTML\n";
    std::cout << "3. Cancel\n";
    int exportChoice = readIntInput("Select export format: > ");

    if (exportChoice == 3) return;

    std::string defaultFilename = (exportChoice == 1) ? "timetable.csv" : "timetable.html";
    std::string filename = readLineInput("Enter filename (Press enter for default: " + defaultFilename + "):\n> ");
    if (filename.empty()) {
        filename = defaultFilename;
    }

    if (exportChoice == 1) {
        ExportService::exportToCSV(filename, timetable, dm);
    } else if (exportChoice == 2) {
        ExportService::exportToHTML(filename, timetable, dm);
    } else {
        std::cout << "Invalid selection. Export cancelled.\n";
    }
}

void Menu::handleRunBenchmark() {
    std::cout << "\nThis will run the benchmark suite with multiple scenarios.\n";
    std::cout << "This may take a few minutes for larger scenarios.\n";
    std::string confirm = readLineInput("Continue? (y/n): > ");
    if (confirm != "y" && confirm != "Y" && confirm != "yes") {
        std::cout << "Benchmark cancelled.\n";
        return;
    }
    Benchmark::runSuite();
}

void Menu::handleAddFixedEvent() {
    std::cout << "\n--- Add Fixed Event ---\n";
    if (dm.periods.empty()) {
        std::cout << "Cannot add fixed event: Please register periods first.\n";
        return;
    }

    // 1. Select Recurrence
    std::cout << "\nRecurrence Types:\n";
    std::cout << "0. NONE\n";
    std::cout << "1. DAILY\n";
    std::cout << "2. WEEKLY\n";
    int recChoice = readIntInput("Select Recurrence Type:\n> ");
    RecurrenceType recurrence = RecurrenceType::NONE;
    if (recChoice == 1) {
        recurrence = RecurrenceType::DAILY;
    } else if (recChoice == 2) {
        recurrence = RecurrenceType::WEEKLY;
    }

    // 2. Select Day (if not DAILY)
    int dayId = -1;
    if (recurrence != RecurrenceType::DAILY) {
        if (dm.days.empty()) {
            std::cout << "Cannot add non-daily fixed event: Please register days first.\n";
            return;
        }
        std::cout << "\nDays\n";
        for (const auto& d : dm.days) {
            std::cout << d.id << ". " << d.name << "\n";
        }
        dayId = readIntInput("Select Day ID:\n> ");
        if (!dm.dayExists(dayId)) {
            std::cout << "Invalid Day ID.\n";
            return;
        }
    }

    // 3. Select Period
    std::cout << "\nPeriods\n";
    for (const auto& p : dm.periods) {
        std::cout << p.id << ". P" << p.id << " (" << p.startTime << "-" << p.endTime << ")\n";
    }
    int periodId = readIntInput("Select Period ID:\n> ");
    if (!dm.periodExists(periodId)) {
        std::cout << "Invalid Period ID.\n";
        return;
    }

    // 4. Enter Name
    std::string name = readLineInput("Enter Event Name (e.g., Lunch):\n> ");
    if (name.empty()) {
        std::cout << "Event name cannot be empty.\n";
        return;
    }

    int id = dm.addFixedEvent(dayId, periodId, name, recurrence);
    if (id > 0) {
        std::cout << "Fixed Event \"" << name << "\" added successfully with ID " << id << ".\n";
    } else {
        std::cout << "Failed to add Fixed Event.\n";
    }
}

void Menu::handleAddTeacherPreference() {
    std::cout << "\n--- Add Teacher Preference ---\n";
    if (dm.teachers.empty() || dm.days.empty() || dm.periods.empty()) {
        std::cout << "Cannot add preference: Please register teachers, days, and periods first.\n";
        return;
    }

    // 1. Select Teacher
    std::cout << "\nTeachers\n";
    for (const auto& t : dm.teachers) {
        std::cout << t.id << ". " << t.name << "\n";
    }
    int teacherId = readIntInput("Select Teacher ID:\n> ");
    if (!dm.teacherExists(teacherId)) {
        std::cout << "Invalid Teacher ID.\n";
        return;
    }

    // 2. Select Day
    std::cout << "\nDays\n";
    for (const auto& d : dm.days) {
        std::cout << d.id << ". " << d.name << "\n";
    }
    int dayId = readIntInput("Select Day ID:\n> ");
    if (!dm.dayExists(dayId)) {
        std::cout << "Invalid Day ID.\n";
        return;
    }

    // 3. Select Period
    std::cout << "\nPeriods\n";
    for (const auto& p : dm.periods) {
        std::cout << p.id << ". P" << p.id << " (" << p.startTime << "-" << p.endTime << ")\n";
    }
    int periodId = readIntInput("Select Period ID:\n> ");
    if (!dm.periodExists(periodId)) {
        std::cout << "Invalid Period ID.\n";
        return;
    }

    // 4. Select Preference Type
    std::cout << "\nPreference Types:\n";
    std::cout << "0. PREFERRED\n";
    std::cout << "1. NEUTRAL\n";
    std::cout << "2. UNDESIRABLE\n";
    int prefChoice = readIntInput("Select Preference Type:\n> ");
    PreferenceType type = PreferenceType::NEUTRAL;
    if (prefChoice == 0) {
        type = PreferenceType::PREFERRED;
    } else if (prefChoice == 2) {
        type = PreferenceType::UNDESIRABLE;
    }

    if (dm.addTeacherPreference(teacherId, dayId, periodId, type)) {
        std::cout << "Teacher preference recorded successfully: Teacher " << dm.getTeacherName(teacherId)
                  << " preference for " << dm.getDayName(dayId) << " P" << periodId 
                  << " set to " << preferenceTypeToString(type) << ".\n";
    } else {
        std::cout << "Failed to record teacher preference.\n";
    }
// View Teacher Timetable
void Menu::handleViewTeacherTimetable() {
    if (!timetableGenerated) {
        std::cout << "\nPlease generate the timetable first (Option 11).\n";
        return;
    }
    std::cout << "\n--- Teacher Timetable ---\n";
    for (const auto& teacher : dm.teachers) {
        std::cout << "\nTeacher: " << teacher.name << " (ID " << teacher.id << ")\n";
        // Header
        std::cout << std::left << std::setw(12) << "DAY";
        for (int p = 0; p < static_cast<int>(dm.periods.size()); ++p) {
            std::cout << std::left << std::setw(28) << ("P" + std::to_string(p+1));
        }
        std::cout << "\n";
        for (int d = 0; d < static_cast<int>(dm.days.size()); ++d) {
            std::cout << std::left << std::setw(12) << dm.getDayName(dm.days[d].id);
            for (int p = 0; p < static_cast<int>(dm.periods.size()); ++p) {
                std::string cellText = "---";
                const auto& cell = timetable.getSlot(dm.classes[0].id, d, p); // placeholder to access method
                // Scan all classes for this teacher at this slot
                for (const auto& pair : timetable.schedules) {
                    const auto& grid = pair.second;
                    if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                        const auto& c = grid[d][p];
                        if (c.teacherId == teacher.id) {
                            cellText = dm.getClassName(pair.first) + " (" + dm.getSubjectName(c.subjectId) + ")";
                            break;
                        }
                    }
                }
                std::cout << std::left << std::setw(28) << cellText;
            }
            std::cout << "\n";
        }
    }
}

// View Room Timetable
void Menu::handleViewRoomTimetable() {
    if (!timetableGenerated) {
        std::cout << "\nPlease generate the timetable first (Option 11).\n";
        return;
    }
    std::cout << "\n--- Room Timetable ---\n";
    for (const auto& room : dm.rooms) {
        std::cout << "\nRoom: " << room.name << " (ID " << room.id << ")\n";
        std::cout << std::left << std::setw(12) << "DAY";
        for (int p = 0; p < static_cast<int>(dm.periods.size()); ++p) {
            std::cout << std::left << std::setw(28) << ("P" + std::to_string(p+1));
        }
        std::cout << "\n";
        for (int d = 0; d < static_cast<int>(dm.days.size()); ++d) {
            std::cout << std::left << std::setw(12) << dm.getDayName(dm.days[d].id);
            for (int p = 0; p < static_cast<int>(dm.periods.size()); ++p) {
                std::string cellText = "---";
                for (const auto& pair : timetable.schedules) {
                    const auto& grid = pair.second;
                    if (d < static_cast<int>(grid.size()) && p < static_cast<int>(grid[d].size())) {
                        const auto& c = grid[d][p];
                        if (c.roomId == room.id) {
                            cellText = dm.getClassName(pair.first) + " (" + dm.getSubjectName(c.subjectId) + ")";
                            break;
                        }
                    }
                }
                std::cout << std::left << std::setw(28) << cellText;
            }
            std::cout << "\n";
        }
    }
}
void Menu::handleViewAnalytics() {
    AnalyticsService analytics;
    AnalyticsReport report = analytics.generateReport(timetable);
    std::cout << "\n--- Analytics Dashboard ---\n";
    std::cout << "Total Scheduled Lessons: " << report.totalLessons << "\n";
    std::cout << "Unscheduled Lessons: " << report.unscheduledLessons << "\n";
    std::cout << "Average Room Utilization: " << report.averageRoomUtilization << "%\n";
    std::cout << "Average Teacher Load: " << report.averageTeacherLoad << " lessons per teacher\n";
    std::cout << "Detected Conflicts (approx): " << report.conflictCount << "\n";
    if (!report.notes.empty()) {
        std::cout << "Notes:\n";
        for (const auto &n : report.notes) {
            std::cout << " - " << n << "\n";
        }
    }
    std::cout << "---------------------------\n";
}

}
