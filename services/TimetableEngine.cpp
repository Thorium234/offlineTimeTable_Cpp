#include "TimetableEngine.h"
#include <iostream>

Timetable TimetableEngine::generate(const DataManager& dm) {
    Timetable timetable;

    // Initialize schedules for all classes
    for (const auto& c : dm.classes) {
        timetable.initClass(c.id);
    }

    // Keep track of teacher availability (teacherId -> 5x8 grid)
    std::map<int, std::vector<std::vector<bool>>> teacherBusy;
    for (const auto& t : dm.teachers) {
        teacherBusy[t.id] = std::vector<std::vector<bool>>(DAYS, std::vector<bool>(PERIODS, false));
    }

    // Allocate lessons to slots
    for (const auto& lesson : dm.lessons) {
        int periodsNeeded = lesson.periodsPerWeek;
        int placed = 0;

        // Try to place the lesson in empty slots
        for (int day = 0; day < DAYS && placed < periodsNeeded; ++day) {
            for (int period = 0; period < PERIODS && placed < periodsNeeded; ++period) {
                // Check if class is free (unassigned slot has "---")
                bool classFree = (timetable.getSlot(lesson.classId, day, period) == "---");

                // Check if teacher is free
                bool teacherFree = false;
                auto tIt = teacherBusy.find(lesson.teacherId);
                if (tIt != teacherBusy.end()) {
                    teacherFree = !tIt->second[day][period];
                } else {
                    // Safe fallback if teacher ID wasn't properly initialized
                    teacherFree = true;
                }

                if (classFree && teacherFree) {
                    // Place the lesson
                    std::string subjectName = dm.getSubjectName(lesson.subjectId);
                    timetable.setSlot(lesson.classId, day, period, subjectName);

                    // Mark teacher as busy
                    if (tIt != teacherBusy.end()) {
                        tIt->second[day][period] = true;
                    }
                    placed++;
                }
            }
        }

        if (placed < periodsNeeded) {
            std::cout << "[Warning] Could only schedule " << placed << "/" << periodsNeeded 
                      << " periods for Subject '" << dm.getSubjectName(lesson.subjectId)
                      << "' in Class '" << dm.getClassName(lesson.classId)
                      << "' taught by Teacher '" << dm.getTeacherName(lesson.teacherId) << "'.\n";
        }
    }

    return timetable;
}
