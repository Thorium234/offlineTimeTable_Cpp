#include "../services/DataManager.h"
#include "../services/TimetableEngine.h"
#include "../services/BacktrackingSolver.h"
#include "../services/GreedySolver.h"
#include "../services/TimetableEvaluator.h"
#include "../models/FixedEvent.h"
#include "../models/TeacherPreference.h"
#include <iostream>
#include <cassert>
#include <stdexcept>

void runRegressionTest() {
    std::cout << "Running Regression Test: Daily Fixed Event (Lunch at P3)..." << std::endl;

    DataManager dm;
    // Clear any prepopulated defaults to start from scratch
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

    // 1. Create Monday-Friday (5 days)
    dm.addDay("Monday");
    dm.addDay("Tuesday");
    dm.addDay("Wednesday");
    dm.addDay("Thursday");
    dm.addDay("Friday");

    // 2. Create P1, P2, P3, P4 (4 periods)
    dm.addPeriod("08:00", "09:00");
    dm.addPeriod("09:00", "10:00");
    dm.addPeriod("10:00", "11:00");
    dm.addPeriod("11:00", "12:00");

    // 3. Add Lunch (DAILY, P3)
    int fixedId = dm.addFixedEvent(-1, 3, "Lunch", RecurrenceType::DAILY);
    assert(fixedId > 0);

    // Verify stored dayId is -1 for DAILY event
    assert(dm.fixedEvents[0].dayId == -1);

    // 4. Add Classroom Room Type
    int classRoomType = dm.addRoomType("Classroom");

    // 5. Add Teacher, Subject, Class, Room
    int teacherId = dm.addTeacher("Math Teacher");
    int subjectId = dm.addSubject("Math");
    dm.setSubjectRequirement(subjectId, classRoomType);
    int classId = dm.addClass("Class A", 30);
    int roomId = dm.addRoom("Room 101", 35, classRoomType);
    (void)roomId;

    // 6. Attempt 20 periods of Math (4 periods per day * 5 days)
    // Block size 1, max per day 0 (no restriction)
    dm.addLesson(teacherId, subjectId, classId, 20, 1, 0);

    // 7. Solve using TimetableEngine (which falls back to Greedy when Backtracking is infeasible)
    TimetableEngine engine;
    Timetable timetable = engine.generate(dm);

    // Assert that no lesson ever appears in P3 (index 2) on any day
    for (int d = 0; d < 5; ++d) {
        TimetableCell cell = timetable.getSlot(classId, d, 2);
        assert(cell.isEmpty());
    }

    // Since P3 is blocked on all 5 days, max slots available is 15.
    // Out of 20 periods, exactly 15 should be scheduled and 5 should be unscheduled.
    std::cout << "Unscheduled lessons count: " << timetable.unscheduledLessons.size() << std::endl;
    assert(timetable.unscheduledLessons.size() == 5);

    // Check GreedySolver directly as well
    std::cout << "Testing GreedySolver directly..." << std::endl;
    GreedySolver greedySolver;
    SolverStats statsGreedy;
    Timetable timetableGreedy = greedySolver.solve(dm, statsGreedy);

    // Assert that no lesson ever appears in P3 (index 2) on any day in Greedy
    for (int d = 0; d < 5; ++d) {
        TimetableCell cell = timetableGreedy.getSlot(classId, d, 2);
        assert(cell.isEmpty());
    }
    assert(timetableGreedy.unscheduledLessons.size() == 5);

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

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "         RUNNING TIMETABLE TESTS        " << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        runRegressionTest();
        runTeacherPreferenceTest();
        runLoudFailTest();
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
