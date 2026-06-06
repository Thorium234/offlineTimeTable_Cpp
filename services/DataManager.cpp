#include "DataManager.h"
#include <algorithm>
#include <sstream>
#include <QString>
#include <QDebug>

DataManager::DataManager() {
    // Prepopulate standard school days
    addDay("Monday");
    addDay("Tuesday");
    addDay("Wednesday");
    addDay("Thursday");
    addDay("Friday");

    // Prepopulate standard school periods
    addPeriod("08:00", "08:45");
    addPeriod("08:45", "09:30");
    addPeriod("09:30", "10:15");
    addPeriod("10:15", "11:00");
    addPeriod("11:00", "11:45");
    addPeriod("12:30", "13:15");
    addPeriod("13:15", "14:00");
    addPeriod("14:00", "14:45");

    // Prepopulate standard room types
    addRoomType("Classroom");
    addRoomType("Science Lab");
    addRoomType("Computer Lab");

    // Open SQLite DB and load persisted entities
    loadFromDB();
}

bool DataManager::loadFromDB()
{
    if (!sql.open()) {
        qWarning() << "Failed to open SQLite DB; starting with empty in-memory data";
        return false;
    }
    // Clear existing in-memory data for entities that come from DB
    teachers.clear();
    subjects.clear();
    classes.clear();
    rooms.clear();
    lessons.clear();
    teacherConstraints.clear();
    teacherPreferences.clear();
    roomTypes.clear();
    fixedEvents.clear();
    subjectRequirements.clear();

    for (const auto &rec : sql.fetchTeachers()) {
        Teacher t;
        t.id = rec.id;
        t.name = rec.name.toStdString();
        t.maxConsecutive = rec.maxConsecutive;
        teachers.push_back(t);
    }
    for (const auto &rec : sql.fetchSubjects()) {
        subjects.push_back(Subject{rec.id, rec.name.toStdString()});
    }
    for (const auto &rec : sql.fetchClasses()) {
        classes.push_back(SchoolClass{rec.id, rec.name.toStdString(), rec.studentCount});
    }
    for (const auto &rec : sql.fetchRooms()) {
        rooms.push_back(Room{rec.id, rec.name.toStdString(), rec.capacity, rec.roomTypeId});
    }
    for (const auto &rec : sql.fetchLessons()) {
        Lesson l;
        l.id = rec.id;
        l.teacherId = rec.teacherId;
        l.secondTeacherId = rec.secondTeacherId;
        l.subjectId = rec.subjectId;
        l.classId = rec.classId;
        l.combinedClassIds = rec.combinedClassIds;
        l.periodsPerWeek = rec.periodsPerWeek;
        l.blockSize = rec.blockSize;
        l.maxPerDay = rec.maxPerDay;
        l.weekType = rec.weekType;
        lessons.push_back(l);
    }
    for (const auto &rec : sql.fetchTeacherConstraints()) {
        teacherConstraints.push_back(TeacherConstraint{rec.teacherId, rec.dayId, rec.periodId});
    }
    for (const auto &rec : sql.fetchTeacherPreferences()) {
        PreferenceType pt = PreferenceType::NEUTRAL;
        if (rec.prefType == "PREFERRED") pt = PreferenceType::PREFERRED;
        else if (rec.prefType == "UNDESIRABLE") pt = PreferenceType::UNDESIRABLE;
        teacherPreferences.push_back(TeacherPreference{rec.teacherId, rec.dayId, rec.periodId, pt});
    }
    for (const auto &rec : sql.fetchRoomTypes()) {
        roomTypes.push_back(RoomType{rec.id, rec.name.toStdString()});
    }
    for (const auto &rec : sql.fetchFixedEvents()) {
        RecurrenceType rt = RecurrenceType::NONE;
        if (rec.recurrence == "DAILY") rt = RecurrenceType::DAILY;
        else if (rec.recurrence == "WEEKLY") rt = RecurrenceType::WEEKLY;
        fixedEvents.push_back(FixedEvent{rec.id, rec.dayId, rec.periodId, rec.name.toStdString(), rt});
    }
    // Load substitutions
    substitutions.clear();
    for (const auto &rec : sql.fetchSubstitutions()) {
        Substitution s;
        s.id = rec.id;
        s.originalTeacherId = rec.originalTeacherId;
        s.substituteTeacherId = rec.substituteTeacherId;
        s.subjectId = rec.subjectId;
        s.classId = rec.classId;
        s.dayId = rec.dayId;
        s.periodId = rec.periodId;
        s.status = substitutionStatusFromString(rec.status.toStdString());
        s.reason = rec.reason.toStdString();
        s.date = rec.date.toStdString();
        substitutions.push_back(s);
    }
    // Load divisions
    divisions.clear();
    for (const auto &rec : sql.fetchDivisions()) {
        divisions.push_back(Division{rec.id, rec.name.toStdString(), {}, rec.canRunInParallel});
    }
    // Assign division members from classes
    for (const auto &c : classes) {
        if (c.divisionId > 0) {
            for (auto &d : divisions) {
                if (d.id == c.divisionId) {
                    d.classIds.push_back(c.id);
                    break;
                }
            }
        }
    }
    return true;
}

int DataManager::addTeacher(const std::string &name) {
    int id = sql.addTeacher(QString::fromStdString(name));
    if (id != -1) {
        teachers.push_back(Teacher{id, name});
    }
    return id;
}

int DataManager::addSubject(const std::string& name) {
    int id = sql.addSubject(QString::fromStdString(name));
    if (id != -1) {
        subjects.push_back(Subject{id, name});
    }
    return id;
}

bool DataManager::updateTeacher(int id, const std::string& newName) {
    if (sql.updateTeacher(id, QString::fromStdString(newName))) {
        for (auto &t : teachers) {
            if (t.id == id) {
                t.name = newName;
                return true;
            }
        }
    }
    return false;
}

bool DataManager::removeTeacher(int id) {
    if (!sql.removeTeacher(id)) return false;
    lessons.erase(std::remove_if(lessons.begin(), lessons.end(),
        [id](const Lesson& l) { return l.teacherId == id || l.secondTeacherId == id; }), lessons.end());
    teacherConstraints.erase(std::remove_if(teacherConstraints.begin(), teacherConstraints.end(),
        [id](const TeacherConstraint& tc) { return tc.teacherId == id; }), teacherConstraints.end());
    teacherPreferences.erase(std::remove_if(teacherPreferences.begin(), teacherPreferences.end(),
        [id](const TeacherPreference& tp) { return tp.teacherId == id; }), teacherPreferences.end());
    teachers.erase(std::remove_if(teachers.begin(), teachers.end(),
        [id](const Teacher& t) { return t.id == id; }), teachers.end());
    return true;
}

bool DataManager::updateSubject(int id, const std::string& newName) {
    if (sql.updateSubject(id, QString::fromStdString(newName))) {
        for (auto &s : subjects) {
            if (s.id == id) {
                s.name = newName;
                return true;
            }
        }
    }
    return false;
}

bool DataManager::removeSubject(int id) {
    if (!sql.removeSubject(id)) return false;
    lessons.erase(std::remove_if(lessons.begin(), lessons.end(),
        [id](const Lesson& l) { return l.subjectId == id; }), lessons.end());
    subjectRequirements.erase(std::remove_if(subjectRequirements.begin(), subjectRequirements.end(),
        [id](const SubjectRequirement& sr) { return sr.subjectId == id; }), subjectRequirements.end());
    subjects.erase(std::remove_if(subjects.begin(), subjects.end(),
        [id](const Subject& s) { return s.id == id; }), subjects.end());
    return true;
}

// Fixed Event CRUD implementations
int DataManager::addFixedEvent(int dayId, int periodId, const std::string& name, RecurrenceType recurrence) {
    // For DAILY events dayId is ignored (all days blocked), but we still validate period
    if (!periodExists(periodId)) {
        return -1; // invalid period
    }
    // For WEEKLY/NONE events, dayId must be valid
    if (recurrence != RecurrenceType::DAILY && !dayExists(dayId)) {
        return -1; // invalid day reference
    }
    int storedDayId = (recurrence == RecurrenceType::DAILY) ? -1 : dayId;
    QString recStr = QString::fromStdString(recurrenceToString(recurrence));
    int id = sql.addFixedEvent(storedDayId, periodId, QString::fromStdString(name), recStr);
    if (id == -1) return -1;
    fixedEvents.push_back(FixedEvent{id, storedDayId, periodId, name, recurrence});
    return id;
}

bool DataManager::removeFixedEvent(int eventId) {
    if (!sql.removeFixedEvent(eventId)) return false;
    for (auto it = fixedEvents.begin(); it != fixedEvents.end(); ++it) {
        if (it->id == eventId) {
            fixedEvents.erase(it);
            return true;
        }
    }
    return false;
}

const std::vector<FixedEvent>& DataManager::getFixedEvents() const {
    return fixedEvents;
}

int DataManager::addClass(const std::string &name, int studentCount) {
    int id = sql.addClass(QString::fromStdString(name), studentCount);
    if (id != -1) {
        classes.push_back(SchoolClass{id, name, studentCount});
    }
    return id;
}

int DataManager::addRoom(const std::string &name, int capacity, int roomTypeId) {
    int id = sql.addRoom(QString::fromStdString(name), capacity, roomTypeId);
    if (id != -1) {
        rooms.push_back(Room{id, name, capacity, roomTypeId});
    }
    return id;
}

bool DataManager::updateClass(int id, const std::string& newName, int studentCount) {
    if (sql.updateClass(id, QString::fromStdString(newName), studentCount)) {
        for (auto &c : classes) {
            if (c.id == id) {
                c.name = newName;
                c.studentCount = studentCount;
                return true;
            }
        }
    }
    return false;
}

bool DataManager::removeClass(int id) {
    if (!sql.removeClass(id)) return false;
    lessons.erase(std::remove_if(lessons.begin(), lessons.end(),
        [id](const Lesson& l) {
            return l.classId == id ||
                   std::find(l.combinedClassIds.begin(), l.combinedClassIds.end(), id) != l.combinedClassIds.end();
        }), lessons.end());
    substitutions.erase(std::remove_if(substitutions.begin(), substitutions.end(),
        [id](const Substitution& s) { return s.classId == id; }), substitutions.end());
    classes.erase(std::remove_if(classes.begin(), classes.end(),
        [id](const SchoolClass& c) { return c.id == id; }), classes.end());
    return true;
}

bool DataManager::updateRoom(int id, const std::string& newName, int capacity, int roomTypeId) {
    if (sql.updateRoom(id, QString::fromStdString(newName), capacity, roomTypeId)) {
        for (auto &r : rooms) {
            if (r.id == id) {
                r.name = newName;
                r.capacity = capacity;
                r.roomTypeId = roomTypeId;
                return true;
            }
        }
    }
    return false;
}

bool DataManager::removeRoom(int id) {
    if (!sql.removeRoom(id)) return false;
    rooms.erase(std::remove_if(rooms.begin(), rooms.end(),
        [id](const Room& r) { return r.id == id; }), rooms.end());
    return true;
}

bool DataManager::addLesson(int teacherId, int subjectId, int classId, int periodsPerWeek,
                             int blockSize, int maxPerDay, int weekType,
                             int secondTeacherId, const std::vector<int>& combinedClassIds) {
    if (!teacherExists(teacherId) || !subjectExists(subjectId) || !classExists(classId)) {
        return false;
    }
    int safeBlockSize = std::max(1, blockSize);
    int id = sql.addLesson(teacherId, subjectId, classId, periodsPerWeek, safeBlockSize, maxPerDay, weekType, secondTeacherId);
    if (id == -1) return false;
    for (int ccid : combinedClassIds) {
        sql.addCombinedClass(id, ccid);
    }
    Lesson l;
    l.id = id;
    l.teacherId = teacherId;
    l.secondTeacherId = secondTeacherId;
    l.subjectId = subjectId;
    l.classId = classId;
    l.combinedClassIds = combinedClassIds;
    l.periodsPerWeek = periodsPerWeek;
    l.blockSize = safeBlockSize;
    l.maxPerDay = maxPerDay;
    l.weekType = weekType;
    lessons.push_back(l);
    return true;
}

bool DataManager::updateLesson(int id, int teacherId, int subjectId, int classId, int periodsPerWeek,
                                int blockSize, int maxPerDay, int weekType,
                                int secondTeacherId, const std::vector<int>& combinedClassIds) {
    if (!teacherExists(teacherId) || !subjectExists(subjectId) || !classExists(classId)) {
        return false;
    }
    int safeBlockSize = std::max(1, blockSize);
    if (!sql.updateLesson(id, teacherId, subjectId, classId, periodsPerWeek, safeBlockSize, maxPerDay, weekType, secondTeacherId))
        return false;
    // Update combined classes
    sql.removeCombinedClasses(id);
    for (int ccid : combinedClassIds) {
        sql.addCombinedClass(id, ccid);
    }
    // Update in-memory
    for (auto& l : lessons) {
        if (l.id == id) {
            l.teacherId = teacherId;
            l.secondTeacherId = secondTeacherId;
            l.subjectId = subjectId;
            l.classId = classId;
            l.combinedClassIds = combinedClassIds;
            l.periodsPerWeek = periodsPerWeek;
            l.blockSize = safeBlockSize;
            l.maxPerDay = maxPerDay;
            l.weekType = weekType;
            break;
        }
    }
    return true;
}

bool DataManager::removeLesson(int id) {
    if (!sql.removeLesson(id)) return false;
    for (auto it = lessons.begin(); it != lessons.end(); ++it) {
        if (it->id == id) {
            lessons.erase(it);
            break;
        }
    }
    return true;
}

int DataManager::addDay(const std::string& name) {
    int nextId = static_cast<int>(days.size()) + 1;
    days.push_back(Day{nextId, name});
    return nextId;
}

int DataManager::addPeriod(const std::string& startTime, const std::string& endTime) {
    int nextId = static_cast<int>(periods.size()) + 1;
    periods.push_back(Period{nextId, startTime, endTime});
    return nextId;
}

bool DataManager::addTeacherConstraint(int teacherId, int dayId, int periodId) {
    if (!teacherExists(teacherId) || !dayExists(dayId) || !periodExists(periodId)) {
        return false;
    }
    if (isTeacherUnavailable(teacherId, dayId, periodId)) {
        return true;
    }
    if (!sql.addTeacherConstraint(teacherId, dayId, periodId)) return false;
    teacherConstraints.push_back(TeacherConstraint{teacherId, dayId, periodId});
    return true;
}

int DataManager::addRoomType(const std::string& name) {
    int nextId = static_cast<int>(roomTypes.size()) + 1;
    roomTypes.push_back(RoomType{nextId, name});
    return nextId;
}

bool DataManager::setSubjectRequirement(int subjectId, int roomTypeId) {
    if (!subjectExists(subjectId) || !roomTypeExists(roomTypeId)) {
        return false;
    }
    for (auto& req : subjectRequirements) {
        if (req.subjectId == subjectId) {
            req.roomTypeId = roomTypeId;
            return true;
        }
    }
    subjectRequirements.push_back(SubjectRequirement{subjectId, roomTypeId});
    return true;
}

std::string DataManager::getTeacherName(int id) const {
    for (const auto& t : teachers) {
        if (t.id == id) return t.name;
    }
    return "Unknown Teacher";
}

std::string DataManager::getSubjectName(int id) const {
    for (const auto& s : subjects) {
        if (s.id == id) return s.name;
    }
    return "Unknown Subject";
}

std::string DataManager::getClassName(int id) const {
    for (const auto& c : classes) {
        if (c.id == id) return c.name;
    }
    return "Unknown Class";
}

std::string DataManager::getRoomName(int id) const {
    for (const auto& r : rooms) {
        if (r.id == id) return r.name;
    }
    return "Unknown Room";
}

std::string DataManager::getDayName(int id) const {
    for (const auto& d : days) {
        if (d.id == id) return d.name;
    }
    return "Unknown Day";
}

std::string DataManager::getPeriodTime(int id) const {
    for (const auto& p : periods) {
        if (p.id == id) return p.startTime + "-" + p.endTime;
    }
    return "Unknown Period";
}

std::string DataManager::getRoomTypeName(int id) const {
    for (const auto& rt : roomTypes) {
        if (rt.id == id) return rt.name;
    }
    return "Unknown Room Type";
}

int DataManager::getSubjectRequiredRoomTypeId(int subjectId) const {
    for (const auto& req : subjectRequirements) {
        if (req.subjectId == subjectId) return req.roomTypeId;
    }
    return 1; // Default to room type 1 ("Classroom")
}

bool DataManager::teacherExists(int id) const {
    for (const auto& t : teachers) {
        if (t.id == id) return true;
    }
    return false;
}

bool DataManager::subjectExists(int id) const {
    for (const auto& s : subjects) {
        if (s.id == id) return true;
    }
    return false;
}

bool DataManager::classExists(int id) const {
    for (const auto& c : classes) {
        if (c.id == id) return true;
    }
    return false;
}

bool DataManager::roomExists(int id) const {
    for (const auto& r : rooms) {
        if (r.id == id) return true;
    }
    return false;
}

bool DataManager::dayExists(int id) const {
    for (const auto& d : days) {
        if (d.id == id) return true;
    }
    return false;
}

bool DataManager::periodExists(int id) const {
    for (const auto& p : periods) {
        if (p.id == id) return true;
    }
    return false;
}

bool DataManager::roomTypeExists(int id) const {
    for (const auto& rt : roomTypes) {
        if (rt.id == id) return true;
    }
    return false;
}

bool DataManager::isTeacherUnavailable(int teacherId, int dayId, int periodId) const {
    for (const auto& tc : teacherConstraints) {
        if (tc.teacherId == teacherId && tc.dayId == dayId && tc.periodId == periodId) {
            return true;
        }
    }
    return false;
}

bool DataManager::addTeacherPreference(int teacherId, int dayId, int periodId, PreferenceType type) {
    if (!teacherExists(teacherId) || !dayExists(dayId) || !periodExists(periodId)) {
        return false;
    }
    QString qt = (type == PreferenceType::PREFERRED) ? "PREFERRED"
                : (type == PreferenceType::UNDESIRABLE) ? "UNDESIRABLE" : "NEUTRAL";
    if (!sql.addTeacherPreference(teacherId, dayId, periodId, qt)) return false;
    // Overwrite existing preference if it exists
    for (auto& tp : teacherPreferences) {
        if (tp.teacherId == teacherId && tp.dayId == dayId && tp.periodId == periodId) {
            tp.type = type;
            return true;
        }
    }
    teacherPreferences.push_back(TeacherPreference{teacherId, dayId, periodId, type});
    return true;
}

PreferenceType DataManager::getTeacherPreference(int teacherId, int dayId, int periodId) const {
    for (const auto& tp : teacherPreferences) {
        if (tp.teacherId == teacherId && tp.dayId == dayId && tp.periodId == periodId) {
            return tp.type;
        }
    }
    return PreferenceType::NEUTRAL;
}

int DataManager::saveCurrentTimetable(const std::string &label) {
    TimetableVersion ver;
    ver.label = label.empty()
        ? ("Version " + std::to_string(savedTimetables.size() + 1))
        : label;
    ver.timetable = lastTimetable;
    savedTimetables.push_back(ver);
    return static_cast<int>(savedTimetables.size()) - 1;
}

bool DataManager::loadTimetableVersion(int index) {
    if (index < 0 || index >= static_cast<int>(savedTimetables.size()))
        return false;
    lastTimetable = savedTimetables[index].timetable;
    timetableGenerated = true;
    return true;
}

bool DataManager::performUndo() {
    if (!undoStack.canUndo()) return false;
    CellSnapshot cmd = undoStack.undo();
    auto it = lastTimetable.schedules.find(cmd.classId);
    if (it != lastTimetable.schedules.end()) {
        if (cmd.dayIndex < static_cast<int>(it->second.size()) &&
            cmd.periodIndex < static_cast<int>(it->second[cmd.dayIndex].size())) {
            it->second[cmd.dayIndex][cmd.periodIndex] = cmd.before;
            hasUnsavedChanges = true;
            return true;
        }
    }
    return false;
}

bool DataManager::performRedo() {
    if (!undoStack.canRedo()) return false;
    CellSnapshot cmd = undoStack.redo();
    auto it = lastTimetable.schedules.find(cmd.classId);
    if (it != lastTimetable.schedules.end()) {
        if (cmd.dayIndex < static_cast<int>(it->second.size()) &&
            cmd.periodIndex < static_cast<int>(it->second[cmd.dayIndex].size())) {
            it->second[cmd.dayIndex][cmd.periodIndex] = cmd.after;
            hasUnsavedChanges = true;
            return true;
        }
    }
    return false;
}

void DataManager::clearUndoHistory() {
    undoStack.clear();
}

bool DataManager::toggleSlotLock(int classId, int dayIndex, int periodIndex) {
    auto it = lastTimetable.schedules.find(classId);
    if (it == lastTimetable.schedules.end()) return false;
    auto &grid = it->second;
    if (dayIndex < 0 || dayIndex >= static_cast<int>(grid.size())) return false;
    if (periodIndex < 0 || periodIndex >= static_cast<int>(grid[dayIndex].size())) return false;

    TimetableCell &cell = grid[dayIndex][periodIndex];
    if (cell.isEmpty()) return false;

    cell.locked = !cell.locked;
    hasUnsavedChanges = true;
    return true;
}

bool DataManager::isSlotLocked(int classId, int dayIndex, int periodIndex) const {
    return lastTimetable.isSlotLocked(classId, dayIndex, periodIndex);
}

bool DataManager::manualMoveSlot(int srcClassId, int srcDay, int srcPeriod,
                                  int dstClassId, int dstDay, int dstPeriod) {
    if (srcClassId == dstClassId && srcDay == dstDay && srcPeriod == dstPeriod) return false;

    // Snapshot both cells before moving
    TimetableCell srcBefore = lastTimetable.getSlot(srcClassId, srcDay, srcPeriod);
    TimetableCell dstBefore = lastTimetable.getSlot(dstClassId, dstDay, dstPeriod);

    if (srcBefore.isEmpty() || srcBefore.locked) return false;
    if (dstBefore.locked) return false;

    // Perform the move
    if (!lastTimetable.moveSlot(srcClassId, srcDay, srcPeriod, dstClassId, dstDay, dstPeriod)) {
        return false;
    }

    TimetableCell srcAfter = lastTimetable.getSlot(srcClassId, srcDay, srcPeriod);
    TimetableCell dstAfter = lastTimetable.getSlot(dstClassId, dstDay, dstPeriod);

    // Record undo info for the source cell
    CellSnapshot srcSnap;
    srcSnap.classId = srcClassId;
    srcSnap.dayIndex = srcDay;
    srcSnap.periodIndex = srcPeriod;
    srcSnap.before = srcBefore;
    srcSnap.after = srcAfter;

    // Record undo info for the destination cell
    CellSnapshot dstSnap;
    dstSnap.classId = dstClassId;
    dstSnap.dayIndex = dstDay;
    dstSnap.periodIndex = dstPeriod;
    dstSnap.before = dstBefore;
    dstSnap.after = dstAfter;

    // Push both as one logical action (destination first for undo order)
    undoStack.push(dstSnap);
    undoStack.push(srcSnap);
    undoStack.setActionLabel("Move slot");

    hasUnsavedChanges = true;
    return true;
}

std::vector<ConflictInfo> DataManager::checkConflicts(int classId, int dayIndex, int periodIndex) const {
    if (!timetableGenerated) return {};
    auto it = lastTimetable.schedules.find(classId);
    if (it == lastTimetable.schedules.end()) return {};
    const auto &grid = it->second;
    if (dayIndex >= static_cast<int>(grid.size()) ||
        periodIndex >= static_cast<int>(grid[dayIndex].size())) return {};
    const auto &cell = grid[dayIndex][periodIndex];
    if (cell.isEmpty()) return {};

    ConflictChecker checker(*this, lastTimetable);
    return checker.checkPlacement(classId, cell.subjectId, cell.teacherId, cell.roomId, dayIndex, periodIndex);
}

int DataManager::addSubstitution(int origTeacherId, int subTeacherId, int subjectId,
                                  int classId, int dayId, int periodId,
                                  const std::string &reason, const std::string &date) {
    int id = sql.addSubstitution(origTeacherId, subTeacherId, subjectId, classId,
                                 dayId, periodId, QString::fromStdString(reason),
                                 QString::fromStdString(date));
    if (id != -1) {
        substitutions.push_back(Substitution{id, origTeacherId, subTeacherId, subjectId,
                                             classId, dayId, periodId,
                                             SubstitutionStatus::PENDING, reason, date});
    }
    return id;
}

bool DataManager::updateSubstitutionStatus(int id, SubstitutionStatus status) {
    if (!sql.updateSubstitutionStatus(id, QString::fromStdString(substitutionStatusToString(status))))
        return false;
    for (auto &s : substitutions) {
        if (s.id == id) {
            s.status = status;
            return true;
        }
    }
    return false;
}

bool DataManager::removeSubstitution(int id) {
    if (!sql.removeSubstitution(id)) return false;
    for (auto it = substitutions.begin(); it != substitutions.end(); ++it) {
        if (it->id == id) {
            substitutions.erase(it);
            return true;
        }
    }
    return false;
}

std::vector<DataManager::SubstituteSuggestion> DataManager::suggestSubstituteTeachers(
    int absentTeacherId, int subjectId, int dayId, int periodId) const {
    std::vector<SubstituteSuggestion> suggestions;

    for (const auto &t : teachers) {
        if (t.id == absentTeacherId) continue;

        int score = 0;
        std::string reason;

        bool teachesSubject = false;
        for (const auto &l : lessons) {
            if (l.teacherId == t.id && l.subjectId == subjectId) {
                teachesSubject = true;
                break;
            }
            if (l.secondTeacherId == t.id && l.subjectId == subjectId) {
                teachesSubject = true;
                break;
            }
        }
        if (teachesSubject) {
            score += 50;
            reason = "Teaches this subject";
        } else {
            reason = "Does not teach this subject";
        }

        bool isFree = true;
        if (timetableGenerated) {
            for (const auto &[cid, grid] : lastTimetable.schedules) {
                if (dayId >= 0 && dayId < static_cast<int>(grid.size()) &&
                    periodId >= 0 && periodId < static_cast<int>(grid[dayId].size())) {
                    const auto &cell = grid[dayId][periodId];
                    if (cell.teacherId == t.id && !cell.isEmpty()) {
                        isFree = false;
                        break;
                    }
                }
            }
        }
        if (isFree) {
            score += 30;
            reason += "; Free at this period";
        } else {
            reason += "; Busy at this period";
        }

        int lessonCount = 0;
        for (const auto &l : lessons) {
            if (l.teacherId == t.id || l.secondTeacherId == t.id) ++lessonCount;
        }
        int workloadBonus = std::max(0, 20 - lessonCount);
        score += workloadBonus;

        suggestions.push_back({t.id, score, reason});
    }

    std::sort(suggestions.begin(), suggestions.end(),
              [](const SubstituteSuggestion &a, const SubstituteSuggestion &b) {
                  return a.score > b.score;
              });

    return suggestions;
}

int DataManager::addDivision(const std::string &name, bool canRunInParallel) {
    int id = sql.addDivision(QString::fromStdString(name), canRunInParallel);
    if (id != -1) {
        divisions.push_back(Division{id, name, {}, canRunInParallel});
    }
    return id;
}

bool DataManager::updateDivision(int id, const std::string &name, bool canRunInParallel) {
    if (!sql.updateDivision(id, QString::fromStdString(name), canRunInParallel))
        return false;
    for (auto &d : divisions) {
        if (d.id == id) {
            d.name = name;
            d.canRunInParallel = canRunInParallel;
            return true;
        }
    }
    return false;
}

bool DataManager::removeDivision(int id) {
    if (!sql.removeDivision(id)) return false;
    for (auto it = divisions.begin(); it != divisions.end(); ++it) {
        if (it->id == id) {
            divisions.erase(it);
            return true;
        }
    }
    return false;
}

bool DataManager::assignClassToDivision(int classId, int divisionId) {
    if (!sql.addClassToDivision(classId, divisionId)) return false;
    for (auto &c : classes) {
        if (c.id == classId) {
            c.divisionId = divisionId;
            return true;
        }
    }
    return false;
}

std::vector<ConflictInfo> DataManager::checkMoveConflicts(int srcClassId, int srcDay, int srcPeriod,
                                                           int dstClassId, int dstDay, int dstPeriod) const {
    if (!timetableGenerated) return {};
    ConflictChecker checker(*this, lastTimetable);
    return checker.checkMove(srcClassId, srcDay, srcPeriod, dstClassId, dstDay, dstPeriod);
}

std::vector<ConstraintViolation> DataManager::getConstraintViolations() const {
    std::vector<ConstraintViolation> violations;

    // 1. Analyze placement reject log for unfilled lessons
    for (const auto& msg : placementRejectLog) {
        ConstraintViolation v;
        v.type = "Placement Failure";
        v.explanation = msg;
        v.recommendation = "Try reducing constraints: remove teacher unavailability, "
                           "add more rooms of required type, or reduce lesson block sizes.";
        violations.push_back(v);
    }

    // 2. Analyze unscheduled lessons from last timetable
    for (const auto& ul : lastTimetable.unscheduledLessons) {
        std::string className = getClassName(ul.classId);
        std::string subjectName = getSubjectName(ul.subjectId);
        std::string teacherName = getTeacherName(ul.teacherId);

        ConstraintViolation v;
        v.type = "Unscheduled Lesson";
        std::ostringstream exp;
        exp << "Lesson " << subjectName << " for " << className
            << " (teacher: " << teacherName
            << ") could not be placed. Missing " << ul.periodsCount << " period(s). "
            << ul.reason;
        v.explanation = exp.str();

        // Generate specific recommendation
        std::ostringstream rec;
        if (ul.reason.find("teacher") != std::string::npos || ul.reason.find("Teacher") != std::string::npos) {
            rec << "Check if " << teacherName << " is overbooked or has too many constraints. "
                << "Consider removing some unavailability slots.";
        } else if (ul.reason.find("room") != std::string::npos || ul.reason.find("Room") != std::string::npos) {
            rec << "Add more rooms of the required type, or reduce the number of lessons "
                << "requiring this room type.";
        } else if (ul.reason.find("consecutive") != std::string::npos) {
            rec << "Increase the max consecutive periods for " << teacherName
                << " or reduce lesson block sizes.";
        } else if (ul.reason.find("block") != std::string::npos || ul.reason.find("Block") != std::string::npos) {
            rec << "Reduce block sizes for " << subjectName
                << " or increase available slots by removing constraints.";
        } else {
            rec << "Try reducing constraints, adding more teachers/subjects, or adjusting "
                << "period counts and block sizes.";
        }
        v.recommendation = rec.str();
        violations.push_back(v);
    }

    // 3. Check for teacher overload (assigned more periods than available slots)
    if (timetableGenerated) {
        for (const auto& t : teachers) {
            int totalAssigned = 0;
            int totalRequired = 0;
            for (const auto& l : lessons) {
                if (l.teacherId == t.id) {
                    totalRequired += l.periodsPerWeek;
                }
            }
            for (const auto& pair : lastTimetable.schedules) {
                for (const auto& day : pair.second) {
                    for (const auto& cell : day) {
                        if (cell.teacherId == t.id) {
                            totalAssigned++;
                        }
                    }
                }
            }
            int totalSlots = static_cast<int>(days.size()) * static_cast<int>(periods.size());
            if (totalRequired > totalSlots) {
                ConstraintViolation v;
                v.type = "Teacher Overload";
                std::ostringstream exp;
                exp << t.name << " requires " << totalRequired
                    << " periods/week but only " << totalSlots
                    << " slots are available.";
                v.explanation = exp.str();
                v.recommendation = "Reduce " + t.name + "'s lesson load or add more days/periods.";
                violations.push_back(v);
            }
        }
    }

    return violations;
}
