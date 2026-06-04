#pragma once

#include <vector>
#include <cstring>
#include "../models/Teacher.h"
#include "../models/Subject.h"
#include "../models/SchoolClass.h"
#include "../models/Room.h"
#include "../models/Lesson.h"
#include "../models/Day.h"
#include "../models/Period.h"
#include "../models/TeacherConstraint.h"
#include "../models/SubjectRequirement.h"
#include "../models/RoomType.h"
#include "../models/FixedEvent.h"
#include "../models/TeacherPreference.h"
#include "SQLiteService.h"
#include "../timetable/Timetable.h"

class DataManager {
public:
    // In‑memory vectors (kept for compatibility)
    std::vector<Teacher> teachers;
    std::vector<Subject> subjects;
    std::vector<SchoolClass> classes;
    std::vector<Room> rooms;
    std::vector<FixedEvent> fixedEvents; // New Fixed Events container
    std::vector<TeacherPreference> teacherPreferences; // Teacher Preferences container
    std::vector<Lesson> lessons;
    std::vector<Day> days;
    std::vector<Period> periods;
    std::vector<TeacherConstraint> teacherConstraints;
    std::vector<SubjectRequirement> subjectRequirements;
    // Container for human‑readable constraint violation messages
    mutable std::vector<std::string> placementRejectLog;
    void logPlacementReject(const std::string& msg) const { placementRejectLog.push_back(msg); }
    const std::vector<std::string>& getPlacementRejectLog() const { return placementRejectLog; }
    std::vector<RoomType> roomTypes;

    // Last generated timetable (accessible by all GUI tabs)
    Timetable lastTimetable;
    bool timetableGenerated = false;

    // Persistence layer
    SQLiteService sql;

    DataManager(); // Constructor to prepopulate defaults / load DB

    // CRUD wrappers – now delegate to SQLite and keep vectors in sync
    int addTeacher(const std::string& name);
    bool updateTeacher(int id, const std::string& newName);
    bool removeTeacher(int id);

    int addSubject(const std::string& name);
    bool updateSubject(int id, const std::string& newName);
    bool removeSubject(int id);

    int addClass(const std::string& name, int studentCount);
    bool updateClass(int id, const std::string& newName, int studentCount);
    bool removeClass(int id);

    int addRoom(const std::string& name, int capacity, int roomTypeId);
    bool updateRoom(int id, const std::string& newName, int capacity, int roomTypeId);
    bool removeRoom(int id);

    bool addLesson(int teacherId, int subjectId, int classId, int periodsPerWeek,
                   int blockSize = 1, int maxPerDay = 0);

    int addDay(const std::string& name);
    int addPeriod(const std::string& startTime, const std::string& endTime);
    bool addTeacherConstraint(int teacherId, int dayId, int periodId);
    
    int addRoomType(const std::string& name);
    bool setSubjectRequirement(int subjectId, int roomTypeId);

    // Fixed Event CRUD operations
    int addFixedEvent(int dayId, int periodId, const std::string& name, RecurrenceType recurrence = RecurrenceType::NONE);
    bool removeFixedEvent(int eventId);
    const std::vector<FixedEvent>& getFixedEvents() const;

    // Teacher Preference CRUD operations
    bool addTeacherPreference(int teacherId, int dayId, int periodId, PreferenceType type);
    PreferenceType getTeacherPreference(int teacherId, int dayId, int periodId) const;

    std::string getTeacherName(int id) const;
    std::string getSubjectName(int id) const;
    std::string getClassName(int id) const;
    std::string getRoomName(int id) const;
    std::string getDayName(int id) const;
    std::string getPeriodTime(int id) const;
    std::string getRoomTypeName(int id) const;
    int getSubjectRequiredRoomTypeId(int subjectId) const;

    bool teacherExists(int id) const;
    bool subjectExists(int id) const;
    bool classExists(int id) const;
    bool roomExists(int id) const;
    bool dayExists(int id) const;
    bool periodExists(int id) const;
    bool roomTypeExists(int id) const;
    bool isTeacherUnavailable(int teacherId, int dayId, int periodId) const;
};
