#include "DataManager.h"
#include <algorithm>

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
}

int DataManager::addTeacher(const std::string& name) {
    int nextId = static_cast<int>(teachers.size()) + 1;
    teachers.push_back(Teacher{nextId, name});
    return nextId;
}

int DataManager::addSubject(const std::string& name) {
    int nextId = static_cast<int>(subjects.size()) + 1;
    subjects.push_back(Subject{nextId, name});
    return nextId;
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
    int nextId = static_cast<int>(fixedEvents.size()) + 1;
    int storedDayId = (recurrence == RecurrenceType::DAILY) ? -1 : dayId;
    fixedEvents.push_back(FixedEvent{nextId, storedDayId, periodId, name, recurrence});
    return nextId;
}

bool DataManager::removeFixedEvent(int eventId) {
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

int DataManager::addClass(const std::string& name, int studentCount) {
    int nextId = static_cast<int>(classes.size()) + 1;
    classes.push_back(SchoolClass{nextId, name, studentCount});
    return nextId;
}

int DataManager::addRoom(const std::string& name, int capacity, int roomTypeId) {
    int nextId = static_cast<int>(rooms.size()) + 1;
    rooms.push_back(Room{nextId, name, capacity, roomTypeId});
    return nextId;
}

bool DataManager::addLesson(int teacherId, int subjectId, int classId, int periodsPerWeek,
                             int blockSize, int maxPerDay) {
    if (!teacherExists(teacherId) || !subjectExists(subjectId) || !classExists(classId)) {
        return false;
    }
    int safeBlockSize = std::max(1, blockSize);
    lessons.push_back(Lesson{teacherId, subjectId, classId, periodsPerWeek, safeBlockSize, maxPerDay});
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
