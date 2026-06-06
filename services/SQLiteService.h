#pragma once

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QString>
#include <vector>

// Simple structs for database rows
struct TeacherRecord {
    int id;
    QString name;
    int maxConsecutive = 0;
};

struct SubjectRecord {
    int id;
    QString name;
};

struct ClassRecord {
    int id;
    QString name;
    int studentCount;
};

struct RoomRecord {
    int id;
    QString name;
    int capacity;
    int roomTypeId;
};

struct LessonRecord {
    int id;
    int teacherId;
    int secondTeacherId = -1;
    int subjectId;
    int classId;
    std::vector<int> combinedClassIds;
    int periodsPerWeek;
    int blockSize;
    int maxPerDay;
    int weekType = 0;
};

struct ConstraintRecord {
    int teacherId;
    int dayId;
    int periodId;
};

struct PreferenceRecord {
    int teacherId;
    int dayId;
    int periodId;
    QString prefType;
};

struct RoomTypeRecord {
    int id;
    QString name;
};

struct FixedEventRecord {
    int id;
    int dayId;
    int periodId;
    QString name;
    QString recurrence;
};

class SQLiteService {
public:
    SQLiteService();
    ~SQLiteService();

    // Open (or create) the database file. Returns true on success.
    bool open(const QString &path = QStringLiteral("data/timetableGen.db"));

    // Table creation – called once after opening.
    bool initSchema();

    // Teacher CRUD
    int addTeacher(const QString &name, int maxConsecutive = 0);
    bool updateTeacher(int id, const QString &newName, int maxConsecutive = -1);
    bool removeTeacher(int id);
    std::vector<TeacherRecord> fetchTeachers();

    // Subject CRUD (similar signatures)
    int addSubject(const QString &name);
    bool updateSubject(int id, const QString &newName);
    bool removeSubject(int id);
    std::vector<SubjectRecord> fetchSubjects();

    // Class CRUD
    int addClass(const QString &name, int studentCount);
    bool updateClass(int id, const QString &newName, int studentCount);
    bool removeClass(int id);
    std::vector<ClassRecord> fetchClasses();

    // Room CRUD
    int addRoom(const QString &name, int capacity, int roomTypeId);
    bool updateRoom(int id, const QString &newName, int capacity, int roomTypeId);
    bool removeRoom(int id);
    std::vector<RoomRecord> fetchRooms();

    // Lesson CRUD
    int addLesson(int teacherId, int subjectId, int classId, int periodsPerWeek, int blockSize, int maxPerDay, int weekType = 0, int secondTeacherId = -1);
    bool updateLesson(int id, int teacherId, int subjectId, int classId, int periodsPerWeek, int blockSize, int maxPerDay, int weekType = 0, int secondTeacherId = -1);
    bool removeLesson(int id);
    std::vector<LessonRecord> fetchLessons();
    bool addCombinedClass(int lessonId, int classId);
    bool removeCombinedClasses(int lessonId);
    std::vector<int> fetchCombinedClassIds(int lessonId);

    // Teacher constraints
    bool addTeacherConstraint(int teacherId, int dayId, int periodId);
    bool removeTeacherConstraint(int teacherId, int dayId, int periodId);
    std::vector<ConstraintRecord> fetchTeacherConstraints();

    // Teacher preferences
    bool addTeacherPreference(int teacherId, int dayId, int periodId, const QString &prefType);
    bool removeTeacherPreference(int teacherId, int dayId, int periodId);
    std::vector<PreferenceRecord> fetchTeacherPreferences();

    // Room types
    std::vector<RoomTypeRecord> fetchRoomTypes();

    // Fixed events
    int addFixedEvent(int dayId, int periodId, const QString &name, const QString &recurrence);
    bool removeFixedEvent(int id);
    std::vector<FixedEventRecord> fetchFixedEvents();

    // Timetable slots
    bool clearTimetableSlots();
    bool addTimetableSlot(int classId, int dayId, int periodId, int subjectId, int teacherId, int roomId);

    // Substitutions
    int addSubstitution(int origTeacherId, int subTeacherId, int subjectId, int classId,
                        int dayId, int periodId, const QString &reason, const QString &date);
    bool updateSubstitutionStatus(int id, const QString &status);
    bool removeSubstitution(int id);
    struct SubstitutionRecord {
        int id;
        int originalTeacherId;
        int substituteTeacherId;
        int subjectId;
        int classId;
        int dayId;
        int periodId;
        QString status;
        QString reason;
        QString date;
    };
    std::vector<SubstitutionRecord> fetchSubstitutions();

    // Divisions
    int addDivision(const QString &name, bool canRunInParallel);
    bool updateDivision(int id, const QString &name, bool canRunInParallel);
    bool removeDivision(int id);
    struct DivisionRecord {
        int id;
        QString name;
        bool canRunInParallel;
    };
    std::vector<DivisionRecord> fetchDivisions();
    bool addClassToDivision(int classId, int divisionId);
    bool removeClassFromDivision(int classId);

    // Expose underlying QSqlDatabase for Qt models
    QSqlDatabase &getDatabase();

private:
    QSqlDatabase db;
    bool exec(const QString &query);
};
