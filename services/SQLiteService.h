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

class SQLiteService {
public:
    SQLiteService();
    ~SQLiteService();

    // Open (or create) the database file. Returns true on success.
    bool open(const QString &path = QStringLiteral("data/timetableGen.db"));

    // Table creation – called once after opening.
    bool initSchema();

    // Teacher CRUD
    int addTeacher(const QString &name);
    bool updateTeacher(int id, const QString &newName);
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

    // Expose underlying QSqlDatabase for Qt models
    QSqlDatabase &getDatabase();

private:
    QSqlDatabase db;
    bool exec(const QString &query);
};
