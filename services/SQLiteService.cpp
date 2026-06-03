#include "SQLiteService.h"
#include "../utils/PathUtil.h"
#include <QVariant>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

SQLiteService::SQLiteService() {}
SQLiteService::~SQLiteService() {
    if (db.isOpen()) {
        db.close();
    }
}

bool SQLiteService::open(const QString &path) {
    QString resolvedPath = PathUtil::resolvePath(path);
    
    // Ensure parent directory exists
    QFileInfo fileInfo(resolvedPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create database directory:" << dir.absolutePath();
        }
    }

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(resolvedPath);
    if (!db.open()) {
        qWarning() << "Failed to open SQLite database:" << db.lastError().text();
        return false;
    }
    return initSchema();
}

bool SQLiteService::exec(const QString &query) {
    QSqlQuery q(db);
    if (!q.exec(query)) {
        qWarning() << "SQL exec error:" << q.lastError().text() << "Query:" << query;
        return false;
    }
    return true;
}

bool SQLiteService::initSchema() {
    // Teachers table
    if (!exec("CREATE TABLE IF NOT EXISTS teachers (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, maxConsecutive INTEGER DEFAULT 0)")) return false;
    // Migration: Add maxConsecutive column if it doesn't exist
    exec("ALTER TABLE teachers ADD COLUMN maxConsecutive INTEGER DEFAULT 0");
    // Subjects table
    if (!exec("CREATE TABLE IF NOT EXISTS subjects (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL)")) return false;
    // Classes table
    if (!exec("CREATE TABLE IF NOT EXISTS classes (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, studentCount INTEGER NOT NULL)")) return false;
    // Rooms table
    if (!exec("CREATE TABLE IF NOT EXISTS rooms (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, capacity INTEGER NOT NULL, roomTypeId INTEGER NOT NULL)")) return false;
    return true;
}

int SQLiteService::addTeacher(const QString &name) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO teachers (name) VALUES (:name)");
    q.bindValue(":name", name);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool SQLiteService::updateTeacher(int id, const QString &newName) {
    QSqlQuery q(db);
    q.prepare("UPDATE teachers SET name = :name WHERE id = :id");
    q.bindValue(":name", newName);
    q.bindValue(":id", id);
    return q.exec();
}

bool SQLiteService::removeTeacher(int id) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM teachers WHERE id = :id");
    q.bindValue(":id", id);
    return q.exec();
}

std::vector<TeacherRecord> SQLiteService::fetchTeachers() {
    std::vector<TeacherRecord> result;
    QSqlQuery q(db);
    q.exec("SELECT id, name FROM teachers ORDER BY id");
    while (q.next()) {
        TeacherRecord rec;
        rec.id = q.value(0).toInt();
        rec.name = q.value(1).toString();
        result.push_back(rec);
    }
    return result;
}

int SQLiteService::addSubject(const QString &name) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO subjects (name) VALUES (:name)");
    q.bindValue(":name", name);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool SQLiteService::updateSubject(int id, const QString &newName) { QSqlQuery q(db); q.prepare("UPDATE subjects SET name = :name WHERE id = :id"); q.bindValue(":name", newName); q.bindValue(":id", id); return q.exec(); }
bool SQLiteService::removeSubject(int id) { QSqlQuery q(db); q.prepare("DELETE FROM subjects WHERE id = :id"); q.bindValue(":id", id); return q.exec(); }
std::vector<SubjectRecord> SQLiteService::fetchSubjects() {
    std::vector<SubjectRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id, name FROM subjects ORDER BY id");
    while(q.next()) {
        SubjectRecord rec{q.value(0).toInt(), q.value(1).toString()};
        r.push_back(rec);
    }
    return r;
}

int SQLiteService::addClass(const QString &name, int studentCount) { QSqlQuery q(db); q.prepare("INSERT INTO classes (name, studentCount) VALUES (:name, :cnt)"); q.bindValue(":name", name); q.bindValue(":cnt", studentCount); if(!q.exec()) return -1; return q.lastInsertId().toInt(); }
bool SQLiteService::updateClass(int id, const QString &newName, int studentCount) { QSqlQuery q(db); q.prepare("UPDATE classes SET name = :name, studentCount = :cnt WHERE id = :id"); q.bindValue(":name", newName); q.bindValue(":cnt", studentCount); q.bindValue(":id", id); return q.exec(); }
bool SQLiteService::removeClass(int id) { QSqlQuery q(db); q.prepare("DELETE FROM classes WHERE id = :id"); q.bindValue(":id", id); return q.exec(); }
std::vector<ClassRecord> SQLiteService::fetchClasses() {
    std::vector<ClassRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id, name, studentCount FROM classes ORDER BY id");
    while(q.next()) {
        ClassRecord rec{q.value(0).toInt(), q.value(1).toString(), q.value(2).toInt()};
        r.push_back(rec);
    }
    return r;
}

int SQLiteService::addRoom(const QString &name, int capacity, int roomTypeId) { QSqlQuery q(db); q.prepare("INSERT INTO rooms (name, capacity, roomTypeId) VALUES (:n, :c, :rt)"); q.bindValue(":n", name); q.bindValue(":c", capacity); q.bindValue(":rt", roomTypeId); if(!q.exec()) return -1; return q.lastInsertId().toInt(); }
bool SQLiteService::updateRoom(int id, const QString &newName, int capacity, int roomTypeId) { QSqlQuery q(db); q.prepare("UPDATE rooms SET name = :n, capacity = :c, roomTypeId = :rt WHERE id = :id"); q.bindValue(":n", newName); q.bindValue(":c", capacity); q.bindValue(":rt", roomTypeId); q.bindValue(":id", id); return q.exec(); }
bool SQLiteService::removeRoom(int id) { QSqlQuery q(db); q.prepare("DELETE FROM rooms WHERE id = :id"); q.bindValue(":id", id); return q.exec(); }
std::vector<RoomRecord> SQLiteService::fetchRooms() {
    std::vector<RoomRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id, name, capacity, roomTypeId FROM rooms ORDER BY id");
    while(q.next()) {
        RoomRecord rec{q.value(0).toInt(), q.value(1).toString(), q.value(2).toInt(), q.value(3).toInt()};
        r.push_back(rec);
    }
    return r;
}
