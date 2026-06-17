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
    // Enable WAL journal mode for concurrent C++/Flask access
    exec("PRAGMA journal_mode=WAL");

    // ── Core tables (CREATE IF NOT EXISTS — safe to re-run) ──────────────────
    if (!exec("CREATE TABLE IF NOT EXISTS teachers (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, maxConsecutive INTEGER DEFAULT 0)")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS subjects (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL)")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS classes (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, studentCount INTEGER NOT NULL)")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS rooms (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, capacity INTEGER NOT NULL, roomTypeId INTEGER NOT NULL)")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS room_types (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL)")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS lessons (id INTEGER PRIMARY KEY AUTOINCREMENT, teacherId INTEGER NOT NULL, secondTeacherId INTEGER DEFAULT -1, subjectId INTEGER NOT NULL, classId INTEGER NOT NULL, periodsPerWeek INTEGER NOT NULL DEFAULT 1, blockSize INTEGER NOT NULL DEFAULT 1, maxPerDay INTEGER NOT NULL DEFAULT 0, weekType INTEGER NOT NULL DEFAULT 0)")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS teacher_constraints (id INTEGER PRIMARY KEY AUTOINCREMENT, teacherId INTEGER NOT NULL, dayId INTEGER NOT NULL, periodId INTEGER NOT NULL, UNIQUE(teacherId, dayId, periodId))")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS teacher_preferences (id INTEGER PRIMARY KEY AUTOINCREMENT, teacherId INTEGER NOT NULL, dayId INTEGER NOT NULL, periodId INTEGER NOT NULL, prefType TEXT NOT NULL DEFAULT 'NEUTRAL', UNIQUE(teacherId, dayId, periodId))")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS fixed_events (id INTEGER PRIMARY KEY AUTOINCREMENT, dayId INTEGER NOT NULL DEFAULT -1, periodId INTEGER NOT NULL, name TEXT NOT NULL DEFAULT '', recurrence TEXT NOT NULL DEFAULT 'NONE')")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS timetable_slots (id INTEGER PRIMARY KEY AUTOINCREMENT, classId INTEGER NOT NULL, dayId INTEGER NOT NULL, periodId INTEGER NOT NULL, subjectId INTEGER, teacherId INTEGER, roomId INTEGER, locked INTEGER NOT NULL DEFAULT 0, UNIQUE(classId, dayId, periodId))")) return false;

    // ── Migration-managed tables (also created here so they exist before applyMigrations runs) ──
    if (!exec("CREATE TABLE IF NOT EXISTS schema_migrations (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE, applied_at TEXT NOT NULL DEFAULT (datetime('now')))")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS days (id INTEGER PRIMARY KEY, name TEXT NOT NULL)")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS periods (id INTEGER PRIMARY KEY, label TEXT NOT NULL, startTime TEXT NOT NULL, endTime TEXT NOT NULL)")) return false;
    if (!exec("CREATE TABLE IF NOT EXISTS subject_requirements (id INTEGER PRIMARY KEY AUTOINCREMENT, subjectId INTEGER NOT NULL UNIQUE, roomTypeId INTEGER NOT NULL DEFAULT 1)")) return false;

    // ── Numbered migration runner ─────────────────────────────────────────────
    applyMigrations();

    // ── Seed days and periods tables on first run ─────────────────────────────
    seedDefaultDays();
    seedDefaultPeriods();

    return true;
}

void SQLiteService::applyMigrations() {
    // The canonical 14-migration list (same order as Flask _apply_migrations).
    // Each entry: { name, sql }
    static const struct { const char *name; const char *sql; } MIGRATIONS[] = {
        { "001_add_timetable_slots_locked",
          "ALTER TABLE timetable_slots ADD COLUMN locked INTEGER NOT NULL DEFAULT 0" },
        { "002_add_teachers_color",
          "ALTER TABLE teachers ADD COLUMN color TEXT NOT NULL DEFAULT ''" },
        { "003_add_subjects_color",
          "ALTER TABLE subjects ADD COLUMN color TEXT NOT NULL DEFAULT ''" },
        { "004_add_classes_divisionId",
          "ALTER TABLE classes ADD COLUMN divisionId INTEGER DEFAULT NULL" },
        { "005_add_classes_groupId",
          "ALTER TABLE classes ADD COLUMN groupId INTEGER DEFAULT NULL" },
        { "006_add_lessons_weekType",
          "ALTER TABLE lessons ADD COLUMN weekType INTEGER DEFAULT 0" },
        { "007_add_lessons_secondTeacherId",
          "ALTER TABLE lessons ADD COLUMN secondTeacherId INTEGER DEFAULT -1" },
        { "008_add_timetable_slots_weekType",
          "ALTER TABLE timetable_slots ADD COLUMN weekType INTEGER NOT NULL DEFAULT 0" },
        { "009_create_lesson_combined_classes",
          "CREATE TABLE IF NOT EXISTS lesson_combined_classes (lessonId INTEGER NOT NULL, classId INTEGER NOT NULL, UNIQUE(lessonId, classId))" },
        { "010_add_classes_earliestPeriod",
          "ALTER TABLE classes ADD COLUMN earliestPeriod INTEGER DEFAULT NULL" },
        { "011_add_classes_latestPeriod",
          "ALTER TABLE classes ADD COLUMN latestPeriod INTEGER DEFAULT NULL" },
        { "012_create_subject_requirements",
          "CREATE TABLE IF NOT EXISTS subject_requirements (id INTEGER PRIMARY KEY AUTOINCREMENT, subjectId INTEGER NOT NULL UNIQUE, roomTypeId INTEGER NOT NULL DEFAULT 1)" },
        { "013_create_days",
          "CREATE TABLE IF NOT EXISTS days (id INTEGER PRIMARY KEY, name TEXT NOT NULL)" },
        { "014_create_periods",
          "CREATE TABLE IF NOT EXISTS periods (id INTEGER PRIMARY KEY, label TEXT NOT NULL, startTime TEXT NOT NULL, endTime TEXT NOT NULL)" },
    };

    for (const auto &m : MIGRATIONS) {
        // Check whether this migration has already been applied
        QSqlQuery check(db);
        check.prepare("SELECT COUNT(*) FROM schema_migrations WHERE name = :name");
        check.bindValue(":name", QString::fromUtf8(m.name));
        check.exec();
        if (check.next() && check.value(0).toInt() > 0) {
            continue; // already applied — skip
        }

        // Apply inside an EXCLUSIVE transaction for concurrency safety
        if (!exec("BEGIN EXCLUSIVE")) {
            qWarning() << "applyMigrations: could not acquire EXCLUSIVE lock for" << m.name;
            continue;
        }

        // Re-check inside the transaction (another process may have applied it between
        // our outer check above and acquiring the lock now)
        QSqlQuery recheck(db);
        recheck.prepare("SELECT COUNT(*) FROM schema_migrations WHERE name = :name");
        recheck.bindValue(":name", QString::fromUtf8(m.name));
        recheck.exec();
        bool alreadyApplied = recheck.next() && recheck.value(0).toInt() > 0;

        if (!alreadyApplied) {
            QSqlQuery mq(db);
            bool ok = mq.exec(QString::fromUtf8(m.sql));
            if (!ok) {
                // ALTER TABLE fails if column already exists — that is acceptable; treat
                // as applied so we do not retry on every startup.
                qWarning() << "Migration" << m.name << "SQL error (column may already exist):"
                           << mq.lastError().text();
            }
            QSqlQuery ins(db);
            ins.prepare("INSERT OR IGNORE INTO schema_migrations (name) VALUES (:name)");
            ins.bindValue(":name", QString::fromUtf8(m.name));
            ins.exec();
        }

        exec("COMMIT");
    }
}

// ── Days / Periods seeding ────────────────────────────────────────────────────

void SQLiteService::seedDefaultDays() {
    QSqlQuery check(db);
    check.exec("SELECT COUNT(*) FROM days");
    if (check.next() && check.value(0).toInt() > 0)
        return; // already seeded

    static const struct { int id; const char *name; } DEFAULT_DAYS[] = {
        { 1, "Monday" },
        { 2, "Tuesday" },
        { 3, "Wednesday" },
        { 4, "Thursday" },
        { 5, "Friday" },
    };
    for (const auto &d : DEFAULT_DAYS) {
        QSqlQuery q(db);
        q.prepare("INSERT OR IGNORE INTO days (id, name) VALUES (:id, :name)");
        q.bindValue(":id", d.id);
        q.bindValue(":name", QString::fromUtf8(d.name));
        if (!q.exec()) {
            qWarning() << "seedDefaultDays: failed to insert day" << d.id
                       << ":" << q.lastError().text();
        }
    }
}

void SQLiteService::seedDefaultPeriods() {
    QSqlQuery check(db);
    check.exec("SELECT COUNT(*) FROM periods");
    if (check.next() && check.value(0).toInt() > 0)
        return; // already seeded

    static const struct { int id; const char *label; const char *start; const char *end; }
    DEFAULT_PERIODS[] = {
        { 1, "Period 1", "08:00", "08:45" },
        { 2, "Period 2", "08:50", "09:35" },
        { 3, "Period 3", "09:40", "10:25" },
        { 4, "Period 4", "10:40", "11:25" },
        { 5, "Period 5", "11:30", "12:15" },
        { 6, "Period 6", "13:00", "14:45" },
    };
    for (const auto &p : DEFAULT_PERIODS) {
        QSqlQuery q(db);
        q.prepare("INSERT OR IGNORE INTO periods (id, label, startTime, endTime) "
                  "VALUES (:id, :label, :start, :end)");
        q.bindValue(":id",    p.id);
        q.bindValue(":label", QString::fromUtf8(p.label));
        q.bindValue(":start", QString::fromUtf8(p.start));
        q.bindValue(":end",   QString::fromUtf8(p.end));
        if (!q.exec()) {
            qWarning() << "seedDefaultPeriods: failed to insert period" << p.id
                       << ":" << q.lastError().text();
        }
    }
}

std::vector<DayRecord> SQLiteService::fetchDays() {
    std::vector<DayRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id, name FROM days ORDER BY id");
    while (q.next()) {
        DayRecord rec;
        rec.id   = q.value(0).toInt();
        rec.name = q.value(1).toString();
        r.push_back(rec);
    }
    return r;
}

std::vector<PeriodRecord> SQLiteService::fetchPeriods() {
    std::vector<PeriodRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id, label, startTime, endTime FROM periods ORDER BY id");
    while (q.next()) {
        PeriodRecord rec;
        rec.id        = q.value(0).toInt();
        rec.label     = q.value(1).toString();
        rec.startTime = q.value(2).toString();
        rec.endTime   = q.value(3).toString();
        r.push_back(rec);
    }
    return r;
}

int SQLiteService::addTeacher(const QString &name, int maxConsecutive) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO teachers (name, maxConsecutive) VALUES (:name, :maxConsecutive)");
    q.bindValue(":name", name);
    q.bindValue(":maxConsecutive", maxConsecutive);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool SQLiteService::updateTeacher(int id, const QString &newName, int maxConsecutive) {
    QSqlQuery q(db);
    if (maxConsecutive >= 0) {
        q.prepare("UPDATE teachers SET name = :name, maxConsecutive = :maxConsecutive WHERE id = :id");
        q.bindValue(":maxConsecutive", maxConsecutive);
    } else {
        q.prepare("UPDATE teachers SET name = :name WHERE id = :id");
    }
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
    q.exec("SELECT id, name, maxConsecutive FROM teachers ORDER BY id");
    while (q.next()) {
        TeacherRecord rec;
        rec.id = q.value(0).toInt();
        rec.name = q.value(1).toString();
        rec.maxConsecutive = q.value(2).toInt();
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

std::vector<LessonRecord> SQLiteService::fetchLessons() {
    std::vector<LessonRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id, teacherId, COALESCE(secondTeacherId,-1), subjectId, classId, periodsPerWeek, blockSize, maxPerDay, weekType FROM lessons ORDER BY id");
    while (q.next()) {
        LessonRecord rec;
        rec.id = q.value(0).toInt();
        rec.teacherId = q.value(1).toInt();
        rec.secondTeacherId = q.value(2).toInt();
        rec.subjectId = q.value(3).toInt();
        rec.classId = q.value(4).toInt();
        rec.periodsPerWeek = q.value(5).toInt();
        rec.blockSize = q.value(6).toInt();
        rec.maxPerDay = q.value(7).toInt();
        rec.weekType = q.value(8).toInt();
        rec.combinedClassIds = fetchCombinedClassIds(rec.id);
        r.push_back(rec);
    }
    return r;
}

std::vector<ConstraintRecord> SQLiteService::fetchTeacherConstraints() {
    std::vector<ConstraintRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT teacherId, dayId, periodId FROM teacher_constraints ORDER BY teacherId, dayId, periodId");
    while (q.next()) {
        ConstraintRecord rec{q.value(0).toInt(), q.value(1).toInt(), q.value(2).toInt()};
        r.push_back(rec);
    }
    return r;
}

std::vector<PreferenceRecord> SQLiteService::fetchTeacherPreferences() {
    std::vector<PreferenceRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT teacherId, dayId, periodId, prefType FROM teacher_preferences ORDER BY teacherId, dayId, periodId");
    while (q.next()) {
        PreferenceRecord rec{q.value(0).toInt(), q.value(1).toInt(), q.value(2).toInt(), q.value(3).toString()};
        r.push_back(rec);
    }
    return r;
}

std::vector<RoomTypeRecord> SQLiteService::fetchRoomTypes() {
    std::vector<RoomTypeRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id, name FROM room_types ORDER BY id");
    while (q.next()) {
        RoomTypeRecord rec{q.value(0).toInt(), q.value(1).toString()};
        r.push_back(rec);
    }
    return r;
}

int SQLiteService::addLesson(int teacherId, int subjectId, int classId, int periodsPerWeek, int blockSize, int maxPerDay, int weekType, int secondTeacherId) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO lessons (teacherId, secondTeacherId, subjectId, classId, periodsPerWeek, blockSize, maxPerDay, weekType) "
              "VALUES (:t, :st, :s, :c, :ppw, :bs, :mpd, :wt)");
    q.bindValue(":t", teacherId);
    q.bindValue(":st", secondTeacherId < 0 ? QVariant(QVariant::Int) : secondTeacherId);
    q.bindValue(":s", subjectId);
    q.bindValue(":c", classId);
    q.bindValue(":ppw", periodsPerWeek);
    q.bindValue(":bs", blockSize);
    q.bindValue(":mpd", maxPerDay);
    q.bindValue(":wt", weekType);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool SQLiteService::updateLesson(int id, int teacherId, int subjectId, int classId, int periodsPerWeek, int blockSize, int maxPerDay, int weekType, int secondTeacherId) {
    QSqlQuery q(db);
    q.prepare("UPDATE lessons SET teacherId=:t, secondTeacherId=:st, subjectId=:s, classId=:c, periodsPerWeek=:ppw, blockSize=:bs, maxPerDay=:mpd, weekType=:wt WHERE id=:id");
    q.bindValue(":t", teacherId);
    q.bindValue(":st", secondTeacherId < 0 ? QVariant(QVariant::Int) : secondTeacherId);
    q.bindValue(":s", subjectId);
    q.bindValue(":c", classId);
    q.bindValue(":ppw", periodsPerWeek);
    q.bindValue(":bs", blockSize);
    q.bindValue(":mpd", maxPerDay);
    q.bindValue(":wt", weekType);
    q.bindValue(":id", id);
    return q.exec();
}

bool SQLiteService::removeLesson(int id) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM lessons WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) return false;
    removeCombinedClasses(id);
    return true;
}

bool SQLiteService::addCombinedClass(int lessonId, int classId) {
    QSqlQuery q(db);
    q.prepare("INSERT OR IGNORE INTO lesson_combined_classes (lessonId, classId) VALUES (:l, :c)");
    q.bindValue(":l", lessonId);
    q.bindValue(":c", classId);
    return q.exec();
}

bool SQLiteService::removeCombinedClasses(int lessonId) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM lesson_combined_classes WHERE lessonId=:l");
    q.bindValue(":l", lessonId);
    return q.exec();
}

std::vector<int> SQLiteService::fetchCombinedClassIds(int lessonId) {
    std::vector<int> ids;
    QSqlQuery q(db);
    q.prepare("SELECT classId FROM lesson_combined_classes WHERE lessonId=:l ORDER BY classId");
    q.bindValue(":l", lessonId);
    if (q.exec()) {
        while (q.next()) {
            ids.push_back(q.value(0).toInt());
        }
    }
    return ids;
}

bool SQLiteService::addTeacherConstraint(int teacherId, int dayId, int periodId) {
    QSqlQuery q(db);
    q.prepare("INSERT OR IGNORE INTO teacher_constraints (teacherId, dayId, periodId) VALUES (:t, :d, :p)");
    q.bindValue(":t", teacherId);
    q.bindValue(":d", dayId);
    q.bindValue(":p", periodId);
    return q.exec();
}

bool SQLiteService::removeTeacherConstraint(int teacherId, int dayId, int periodId) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM teacher_constraints WHERE teacherId = :t AND dayId = :d AND periodId = :p");
    q.bindValue(":t", teacherId);
    q.bindValue(":d", dayId);
    q.bindValue(":p", periodId);
    return q.exec();
}

bool SQLiteService::addTeacherPreference(int teacherId, int dayId, int periodId, const QString &prefType) {
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO teacher_preferences (teacherId, dayId, periodId, prefType) "
              "VALUES (:t, :d, :p, :pt)");
    q.bindValue(":t", teacherId);
    q.bindValue(":d", dayId);
    q.bindValue(":p", periodId);
    q.bindValue(":pt", prefType);
    return q.exec();
}

bool SQLiteService::removeTeacherPreference(int teacherId, int dayId, int periodId) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM teacher_preferences WHERE teacherId = :t AND dayId = :d AND periodId = :p");
    q.bindValue(":t", teacherId);
    q.bindValue(":d", dayId);
    q.bindValue(":p", periodId);
    return q.exec();
}

int SQLiteService::addFixedEvent(int dayId, int periodId, const QString &name, const QString &recurrence) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO fixed_events (dayId, periodId, name, recurrence) VALUES (:d, :p, :n, :r)");
    q.bindValue(":d", dayId);
    q.bindValue(":p", periodId);
    q.bindValue(":n", name);
    q.bindValue(":r", recurrence);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool SQLiteService::removeFixedEvent(int id) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM fixed_events WHERE id = :id");
    q.bindValue(":id", id);
    return q.exec();
}

std::vector<FixedEventRecord> SQLiteService::fetchFixedEvents() {
    std::vector<FixedEventRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id, dayId, periodId, name, recurrence FROM fixed_events ORDER BY id");
    while (q.next()) {
        FixedEventRecord rec{q.value(0).toInt(), q.value(1).toInt(), q.value(2).toInt(), q.value(3).toString(), q.value(4).toString()};
        r.push_back(rec);
    }
    return r;
}

// ── Substitutions ────────────────────────────────────────────────────────────

int SQLiteService::addSubstitution(int origTeacherId, int subTeacherId, int subjectId,
                                   int classId, int dayId, int periodId,
                                   const QString &reason, const QString &date) {
    if (!exec("CREATE TABLE IF NOT EXISTS substitutions ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "originalTeacherId INTEGER NOT NULL,"
              "substituteTeacherId INTEGER NOT NULL DEFAULT -1,"
              "subjectId INTEGER NOT NULL,"
              "classId INTEGER NOT NULL,"
              "dayId INTEGER NOT NULL,"
              "periodId INTEGER NOT NULL,"
              "status TEXT NOT NULL DEFAULT 'PENDING',"
              "reason TEXT DEFAULT '',"
              "date TEXT DEFAULT '')")) return -1;
    QSqlQuery q(db);
    q.prepare("INSERT INTO substitutions (originalTeacherId,substituteTeacherId,subjectId,"
              "classId,dayId,periodId,status,reason,date) "
              "VALUES (:o,:s,:sub,:c,:d,:p,:st,:r,:dt)");
    q.bindValue(":o", origTeacherId);
    q.bindValue(":s", subTeacherId);
    q.bindValue(":sub", subjectId);
    q.bindValue(":c", classId);
    q.bindValue(":d", dayId);
    q.bindValue(":p", periodId);
    q.bindValue(":st", QStringLiteral("PENDING"));
    q.bindValue(":r", reason);
    q.bindValue(":dt", date);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool SQLiteService::updateSubstitutionStatus(int id, const QString &status) {
    QSqlQuery q(db);
    q.prepare("UPDATE substitutions SET status=:st WHERE id=:id");
    q.bindValue(":st", status);
    q.bindValue(":id", id);
    return q.exec();
}

bool SQLiteService::removeSubstitution(int id) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM substitutions WHERE id=:id");
    q.bindValue(":id", id);
    return q.exec();
}

std::vector<SQLiteService::SubstitutionRecord> SQLiteService::fetchSubstitutions() {
    exec("CREATE TABLE IF NOT EXISTS substitutions ("
         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
         "originalTeacherId INTEGER NOT NULL,"
         "substituteTeacherId INTEGER NOT NULL DEFAULT -1,"
         "subjectId INTEGER NOT NULL,"
         "classId INTEGER NOT NULL,"
         "dayId INTEGER NOT NULL,"
         "periodId INTEGER NOT NULL,"
         "status TEXT NOT NULL DEFAULT 'PENDING',"
         "reason TEXT DEFAULT '',"
         "date TEXT DEFAULT '')");
    std::vector<SubstitutionRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id,originalTeacherId,substituteTeacherId,subjectId,classId,"
           "dayId,periodId,status,reason,date FROM substitutions ORDER BY id");
    while (q.next()) {
        SubstitutionRecord rec;
        rec.id = q.value(0).toInt();
        rec.originalTeacherId = q.value(1).toInt();
        rec.substituteTeacherId = q.value(2).toInt();
        rec.subjectId = q.value(3).toInt();
        rec.classId = q.value(4).toInt();
        rec.dayId = q.value(5).toInt();
        rec.periodId = q.value(6).toInt();
        rec.status = q.value(7).toString();
        rec.reason = q.value(8).toString();
        rec.date = q.value(9).toString();
        r.push_back(rec);
    }
    return r;
}

// ── Divisions ─────────────────────────────────────────────────────────────────

int SQLiteService::addDivision(const QString &name, bool canRunInParallel) {
    if (!exec("CREATE TABLE IF NOT EXISTS divisions ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT,"
              "name TEXT NOT NULL,"
              "canRunInParallel INTEGER NOT NULL DEFAULT 1)")) return -1;
    QSqlQuery q(db);
    q.prepare("INSERT INTO divisions (name,canRunInParallel) VALUES (:n,:p)");
    q.bindValue(":n", name);
    q.bindValue(":p", canRunInParallel ? 1 : 0);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool SQLiteService::updateDivision(int id, const QString &name, bool canRunInParallel) {
    QSqlQuery q(db);
    q.prepare("UPDATE divisions SET name=:n, canRunInParallel=:p WHERE id=:id");
    q.bindValue(":n", name);
    q.bindValue(":p", canRunInParallel ? 1 : 0);
    q.bindValue(":id", id);
    return q.exec();
}

bool SQLiteService::removeDivision(int id) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM divisions WHERE id=:id");
    q.bindValue(":id", id);
    if (!q.exec()) return false;
    QSqlQuery q2(db);
    q2.prepare("UPDATE classes SET divisionId=NULL WHERE divisionId=:id");
    q2.bindValue(":id", id);
    q2.exec();
    return true;
}

std::vector<SQLiteService::DivisionRecord> SQLiteService::fetchDivisions() {
    exec("CREATE TABLE IF NOT EXISTS divisions ("
         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
         "name TEXT NOT NULL,"
         "canRunInParallel INTEGER NOT NULL DEFAULT 1)");
    std::vector<DivisionRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id,name,canRunInParallel FROM divisions ORDER BY name");
    while (q.next()) {
        DivisionRecord rec;
        rec.id = q.value(0).toInt();
        rec.name = q.value(1).toString();
        rec.canRunInParallel = q.value(2).toInt() != 0;
        r.push_back(rec);
    }
    return r;
}

bool SQLiteService::addClassToDivision(int classId, int divisionId) {
    QSqlQuery q(db);
    q.prepare("UPDATE classes SET divisionId=:d WHERE id=:c");
    q.bindValue(":d", divisionId);
    q.bindValue(":c", classId);
    return q.exec();
}

bool SQLiteService::removeClassFromDivision(int classId) {
    QSqlQuery q(db);
    q.prepare("UPDATE classes SET divisionId=NULL WHERE id=:c");
    q.bindValue(":c", classId);
    return q.exec();
}

bool SQLiteService::clearTimetableSlots() {
    return exec("DELETE FROM timetable_slots");
}

bool SQLiteService::addTimetableSlot(int classId, int dayId, int periodId, int subjectId, int teacherId, int roomId) {
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO timetable_slots (classId, dayId, periodId, subjectId, teacherId, roomId) "
              "VALUES (:classId, :dayId, :periodId, :subjectId, :teacherId, :roomId)");
    q.bindValue(":classId", classId);
    q.bindValue(":dayId", dayId);
    q.bindValue(":periodId", periodId);
    q.bindValue(":subjectId", subjectId);
    q.bindValue(":teacherId", teacherId);
    q.bindValue(":roomId", roomId);
    return q.exec();
}

std::vector<SubjectRequirementRecord> SQLiteService::fetchSubjectRequirements() {
    std::vector<SubjectRequirementRecord> r;
    QSqlQuery q(db);
    q.exec("SELECT id, subjectId, roomTypeId FROM subject_requirements ORDER BY subjectId");
    while (q.next()) {
        SubjectRequirementRecord rec;
        rec.id         = q.value(0).toInt();
        rec.subjectId  = q.value(1).toInt();
        rec.roomTypeId = q.value(2).toInt();
        r.push_back(rec);
    }
    return r;
}
