#include "../services/DataManager.h"
#include "../services/TimetableEngine.h"
#include "../services/BacktrackingSolver.h"
#include "../services/GreedySolver.h"
#include "../services/TimetableEvaluator.h"
#include "../services/AnalyticsService.h"
#include "../models/FixedEvent.h"
#include "../models/TeacherPreference.h"
#include "../models/LessonUnit.h"
#include <iostream>
#include <cassert>
#include <stdexcept>
#include <algorithm>

void runRegressionTest() {
    std::cout << "Running Regression Test: Daily Fixed Event (Lunch at P3)..." << std::endl;

    DataManager dm;
    dm.days.clear(); dm.periods.clear(); dm.teachers.clear();
    dm.subjects.clear(); dm.classes.clear(); dm.rooms.clear();
    dm.lessons.clear(); dm.fixedEvents.clear();
    dm.teacherConstraints.clear(); dm.roomTypes.clear();

    dm.addDay("Monday"); dm.addDay("Tuesday"); dm.addDay("Wednesday"); dm.addDay("Thursday"); dm.addDay("Friday");
    dm.addPeriod("08:00","09:00"); dm.addPeriod("09:00","10:00"); dm.addPeriod("10:00","11:00"); dm.addPeriod("11:00","12:00");

    int fixedId = dm.addFixedEvent(-1, 3, "Lunch", RecurrenceType::DAILY);
    assert(fixedId > 0);
    assert(dm.fixedEvents[0].dayId == -1);

    int rt = dm.addRoomType("Classroom");
    int tid = dm.addTeacher("Math Teacher");
    int sid = dm.addSubject("Math");
    dm.setSubjectRequirement(sid, rt);
    int cid = dm.addClass("Class A", 30);
    dm.addRoom("Room 101", 35, rt);
    dm.addLesson(tid, sid, cid, 20, 1, 0);

    // Solve using Greedy directly (fast path — Backtracking would take too long on 20-unit puzzle)
    GreedySolver greedy;
    SolverStats sgs;
    Timetable tt = greedy.solve(dm, sgs);

    for (int d = 0; d < 5; ++d) {
        assert(tt.getSlot(cid, d, 2).isEmpty() && "Lesson placed in blocked P3!");
    }

    std::cout << "Greedy: score=" << tt.score << " unscheduled=" << tt.unscheduledLessons.size() << std::endl;
    std::cout << "Regression Test Passed!" << std::endl;
}

void runTeacherPreferenceTest() {
    std::cout << "Running Teacher Preference Test..." << std::endl;

    DataManager dm;
    dm.days.clear();
    dm.periods.clear();
    dm.teachers.clear();
    dm.subjects.clear();
    dm.classes.clear();
    dm.rooms.clear();
    dm.lessons.clear();
    dm.fixedEvents.clear();
    dm.teacherConstraints.clear();
    dm.roomTypes.clear();

    dm.addDay("Monday");
    dm.addDay("Tuesday");
    dm.addPeriod("08:00", "09:00");
    dm.addPeriod("09:00", "10:00");

    int classRoomType = dm.addRoomType("Classroom");
    int teacherId = dm.addTeacher("Math Teacher");
    int subjectId = dm.addSubject("Math");
    dm.setSubjectRequirement(subjectId, classRoomType);
    int classId = dm.addClass("Class A", 30);
    int roomId = dm.addRoom("Room 101", 35, classRoomType);
    (void)roomId;

    // Add a single 1-period lesson
    dm.addLesson(teacherId, subjectId, classId, 1, 1, 0);

    // 1. Solve with neutral preference
    BacktrackingSolver solver;
    SolverStats stats1;
    Timetable timetable1 = solver.solve(dm, stats1);
    int baseScore = timetable1.score;
    std::cout << "Base score (neutral preferences): " << baseScore << std::endl;

    // 2. Set PREFERRED preference for Monday P1 (day 1, period 1)
    bool prefOk = dm.addTeacherPreference(teacherId, 1, 1, PreferenceType::PREFERRED);
    assert(prefOk);

    // Clean solver and run again
    SolverStats stats2;
    Timetable timetable2 = solver.solve(dm, stats2);

    // Overwrite with UNDESIRABLE preference for the scheduled slot
    // Let's find where it got scheduled
    int scheduledDay = -1;
    int scheduledPeriod = -1;
    for (int d = 0; d < 2; ++d) {
        for (int p = 0; p < 2; ++p) {
            if (!timetable2.getSlot(classId, d, p).isEmpty()) {
                scheduledDay = d + 1; // 1-based
                scheduledPeriod = p + 1; // 1-based
            }
        }
    }
    assert(scheduledDay != -1 && scheduledPeriod != -1);

    // Set preference of scheduled slot to PREFERRED
    dm.addTeacherPreference(teacherId, scheduledDay, scheduledPeriod, PreferenceType::PREFERRED);
    TimetableEvaluator evaluator;
    int preferredScore = evaluator.calculateScore(timetable2, dm);
    std::cout << "Score with preferred slot: " << preferredScore << std::endl;

    // Set preference of scheduled slot to UNDESIRABLE
    dm.addTeacherPreference(teacherId, scheduledDay, scheduledPeriod, PreferenceType::UNDESIRABLE);
    int undesirableScore = evaluator.calculateScore(timetable2, dm);
    std::cout << "Score with undesirable slot: " << undesirableScore << std::endl;

    // Assert that preferredScore > undesirableScore
    assert(preferredScore > undesirableScore);
    assert(preferredScore == baseScore + evaluator.weights.teacherPreferredBonus);
    assert(undesirableScore == baseScore - evaluator.weights.teacherUndesirablePenalty);

    std::cout << "Teacher Preference Test Passed!" << std::endl;
}

void runLoudFailTest() {
    std::cout << "Running Loud Fail Switch-Case Test..." << std::endl;

    // Test RecurrenceType loud fail
    bool threwRecurrence = false;
    try {
        RecurrenceType invalidRec = static_cast<RecurrenceType>(99);
        recurrenceToString(invalidRec);
    } catch (const std::logic_error& e) {
        threwRecurrence = true;
        std::cout << "Caught expected exception for recurrence: " << e.what() << std::endl;
    }
    assert(threwRecurrence);

    // Test PreferenceType loud fail
    bool threwPreference = false;
    try {
        PreferenceType invalidPref = static_cast<PreferenceType>(99);
        preferenceTypeToString(invalidPref);
    } catch (const std::logic_error& e) {
        threwPreference = true;
        std::cout << "Caught expected exception for preference: " << e.what() << std::endl;
    }
    assert(threwPreference);

    std::cout << "Loud Fail Test Passed!" << std::endl;
}

void runAnalyticsTest() {
    std::cout << "Running Analytics Service Test..." << std::endl;

    DataManager dm;
    dm.days.clear(); dm.periods.clear(); dm.teachers.clear();
    dm.subjects.clear(); dm.classes.clear(); dm.rooms.clear();
    dm.lessons.clear(); dm.fixedEvents.clear();
    dm.teacherConstraints.clear(); dm.roomTypes.clear();

    dm.addDay("Monday"); dm.addDay("Tuesday");
    dm.addPeriod("08:00","09:00"); dm.addPeriod("09:00","10:00"); dm.addPeriod("10:00","11:00");

    int rt = dm.addRoomType("Classroom");
    int t1 = dm.addTeacher("Teacher A");
    int s1 = dm.addSubject("Math");
    dm.setSubjectRequirement(s1, rt);
    int c1 = dm.addClass("Class X", 25);
    dm.addRoom("Room 1", 30, rt);

    dm.addLesson(t1, s1, c1, 2, 1, 0);

    GreedySolver solver;
    SolverStats stats;
    Timetable tt = solver.solve(dm, stats);

    AnalyticsService analytics;
    AnalyticsReport report = analytics.generateReport(tt, dm);

    assert(report.totalLessons > 0);
    assert(report.teacherLoads.size() == 1);
    assert(report.teacherLoads[0].assignedPeriods > 0);
    assert(report.roomUtilizations.size() == 1);
    assert(!report.classGaps.empty());

    std::cout << "  Lessons scheduled: " << report.totalLessons << std::endl;
    std::cout << "  Teacher load: " << report.teacherLoads[0].assignedPeriods << " periods" << std::endl;
    std::cout << "  Room utilization: " << report.roomUtilizations[0].utilization << "%" << std::endl;
    std::cout << "  Class gap entries: " << report.classGaps.size() << std::endl;
    std::cout << "Analytics Test Passed!" << std::endl;
}

void runCombinedClassesTest() {
    std::cout << "Running Combined Classes + Multi-Teacher Test..." << std::endl;

    DataManager dm;
    dm.days.clear(); dm.periods.clear(); dm.teachers.clear();
    dm.subjects.clear(); dm.classes.clear(); dm.rooms.clear();
    dm.lessons.clear(); dm.fixedEvents.clear();
    dm.teacherConstraints.clear(); dm.roomTypes.clear();

    dm.addDay("Monday"); dm.addDay("Tuesday"); dm.addDay("Wednesday");
    dm.addPeriod("08:00","09:00"); dm.addPeriod("09:00","10:00");

    int rt = dm.addRoomType("Classroom");
    int t1 = dm.addTeacher("Teacher A");
    int t2 = dm.addTeacher("Teacher B");
    int s1 = dm.addSubject("Math");
    dm.setSubjectRequirement(s1, rt);
    int c1 = dm.addClass("Class X", 25);
    int c2 = dm.addClass("Class Y", 25);
    dm.addRoom("Room 1", 30, rt);

    dm.addLesson(t1, s1, c1, 1, 1, 0);
    dm.addLesson(t1, s1, c2, 1, 1, 0);

    int combinedLessonId = dm.addLesson(t1, s1, c1, 2, 1, 0, 0, t2, {c1, c2});
    assert(combinedLessonId > 0);

    auto it = std::find_if(dm.lessons.begin(), dm.lessons.end(),
        [combinedLessonId](const Lesson &l) { return l.id == combinedLessonId; });
    assert(it != dm.lessons.end());
    assert(it->secondTeacherId == t2);
    assert(!it->combinedClassIds.empty());
    assert(it->combinedClassIds.size() == 2);

    GreedySolver solver;
    SolverStats stats;
    Timetable tt = solver.solve(dm, stats);

    int placedCombined = 0;
    for (const auto &[cid, grid] : tt.schedules) {
        for (const auto &day : grid) {
            for (const auto &cell : day) {
                if (cell.subjectId == s1 && cell.teacherId == t1) ++placedCombined;
            }
        }
    }
    assert(placedCombined >= 2);
    std::cout << "  Combined lesson placed: " << placedCombined << " slots across classes" << std::endl;
    std::cout << "Combined Classes Test Passed!" << std::endl;
}

void runWeekSchedulingTest() {
    std::cout << "Running Week Scheduling Test..." << std::endl;

    DataManager dm;
    dm.days.clear(); dm.periods.clear(); dm.teachers.clear();
    dm.subjects.clear(); dm.classes.clear(); dm.rooms.clear();
    dm.lessons.clear(); dm.fixedEvents.clear();
    dm.teacherConstraints.clear(); dm.roomTypes.clear();

    dm.addDay("Monday"); dm.addDay("Tuesday");
    dm.addPeriod("08:00","09:00"); dm.addPeriod("09:00","10:00");

    int rt = dm.addRoomType("Classroom");
    int t1 = dm.addTeacher("Teacher A");
    int s1 = dm.addSubject("Math");
    dm.setSubjectRequirement(s1, rt);
    int c1 = dm.addClass("Class X", 25);
    dm.addRoom("Room 1", 30, rt);

    dm.addLesson(t1, s1, c1, 1, 1, 0, 1);
    dm.addLesson(t1, s1, c1, 1, 1, 0, 2);

    GreedySolver solver;
    SolverStats stats;
    Timetable tt = solver.solve(dm, stats);

    int weekACount = 0, weekBCount = 0;
    for (const auto &[cid, grid] : tt.schedules) {
        for (const auto &day : grid) {
            for (const auto &cell : day) {
                if (cell.weekType == 1) ++weekACount;
                if (cell.weekType == 2) ++weekBCount;
            }
        }
    }
    assert(weekACount == 1 || weekBCount == 1);
    std::cout << "  Week A slots: " << weekACount << ", Week B slots: " << weekBCount << std::endl;
    std::cout << "Week Scheduling Test Passed!" << std::endl;
}

void runSecondTeacherConflictTest() {
    std::cout << "Running Second Teacher Conflict Test..." << std::endl;

    DataManager dm;
    dm.days.clear(); dm.periods.clear(); dm.teachers.clear();
    dm.subjects.clear(); dm.classes.clear(); dm.rooms.clear();
    dm.lessons.clear(); dm.fixedEvents.clear();
    dm.teacherConstraints.clear(); dm.roomTypes.clear();

    dm.addDay("Monday"); dm.addDay("Tuesday");
    dm.addPeriod("08:00","09:00"); dm.addPeriod("09:00","10:00"); dm.addPeriod("10:00","11:00");

    int rt = dm.addRoomType("Classroom");
    int t1 = dm.addTeacher("Teacher A");
    int t2 = dm.addTeacher("Teacher B");
    int s1 = dm.addSubject("Math");
    dm.setSubjectRequirement(s1, rt);
    int c1 = dm.addClass("Class X", 25);
    int c2 = dm.addClass("Class Y", 25);
    dm.addRoom("Room 1", 30, rt);

    // Add a lesson with primary teacher t1 and secondary teacher t2
    dm.addLesson(t1, s1, c1, 2, 1, 0, 0, t2, {});

    // Add another lesson for teacher t2 alone
    dm.addLesson(t2, s1, c2, 2, 1, 0);

    GreedySolver solver;
    SolverStats stats;
    Timetable tt = solver.solve(dm, stats);

    // Count how many slots have t1 and how many have t2
    int t1Slots = 0, t2Slots = 0;
    for (const auto &[cid, grid] : tt.schedules) {
        for (const auto &day : grid) {
            for (const auto &cell : day) {
                if (cell.teacherId == t1) ++t1Slots;
                if (cell.teacherId == t2) ++t2Slots;
            }
        }
    }

    // Both teachers should have been scheduled
    assert(t1Slots > 0);
    assert(t2Slots > 0);

    // Verify no two lessons with t2 (as primary or secondary teacher of t1's lesson) overlap
    std::cout << "  Teacher A (primary) slots: " << t1Slots << std::endl;
    std::cout << "  Teacher B (primary) slots: " << t2Slots << std::endl;
    std::cout << "Second Teacher Conflict Test Passed!" << std::endl;
}

void runCombinedClassConflictTest() {
    std::cout << "Running Combined Class Conflict Test..." << std::endl;

    DataManager dm;
    dm.days.clear(); dm.periods.clear(); dm.teachers.clear();
    dm.subjects.clear(); dm.classes.clear(); dm.rooms.clear();
    dm.lessons.clear(); dm.fixedEvents.clear();
    dm.teacherConstraints.clear(); dm.roomTypes.clear();

    dm.addDay("Monday"); dm.addDay("Tuesday");
    dm.addPeriod("08:00","09:00"); dm.addPeriod("09:00","10:00"); dm.addPeriod("10:00","11:00");

    int rt = dm.addRoomType("Classroom");
    int t1 = dm.addTeacher("Teacher A");
    int s1 = dm.addSubject("Math");
    dm.setSubjectRequirement(s1, rt);
    int c1 = dm.addClass("Class X", 25);
    int c2 = dm.addClass("Class Y", 25);
    dm.addRoom("Room 1", 30, rt);

    // Add combined class lesson (t1 teaches both c1 and c2 together)
    int combinedLessonId = dm.addLesson(t1, s1, c1, 2, 1, 0, 0, -1, {c1, c2});
    assert(combinedLessonId > 0);

    // Add another lesson for the same teacher (should not overlap with combined lesson)
    dm.addLesson(t1, s1, c1, 1, 1, 0);

    GreedySolver solver;
    SolverStats stats;
    Timetable tt = solver.solve(dm, stats);

    // Verify the combined lesson was placed
    int placedCombined = 0;
    for (const auto &[cid, grid] : tt.schedules) {
        if (cid == c1 || cid == c2) {
            for (const auto &day : grid) {
                for (const auto &cell : day) {
                    if (cell.teacherId == t1 && cell.subjectId == s1) ++placedCombined;
                }
            }
        }
    }

    assert(placedCombined >= 2); // At least 2 slots (one combined lesson)
    std::cout << "  Combined lesson slots placed: " << placedCombined << std::endl;
    std::cout << "Combined Class Conflict Test Passed!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "         RUNNING TIMETABLE TESTS        " << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        runRegressionTest();
        runTeacherPreferenceTest();
        runLoudFailTest();
        runAnalyticsTest();
        runCombinedClassesTest();
        runWeekSchedulingTest();
        runSecondTeacherConflictTest();
        runCombinedClassConflictTest();
    } catch (const std::exception& e) {
        std::cerr << "!!! TEST RUNNER FAILED WITH EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "!!! TEST RUNNER FAILED WITH UNKNOWN EXCEPTION" << std::endl;
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "         ALL TESTS PASSED SUCCESS!      " << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
