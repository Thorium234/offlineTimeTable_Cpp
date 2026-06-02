#include "DataManager.h"
#include <algorithm>

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

int DataManager::addClass(const std::string& name) {
    int nextId = static_cast<int>(classes.size()) + 1;
    classes.push_back(SchoolClass{nextId, name});
    return nextId;
}

int DataManager::addRoom(const std::string& name) {
    int nextId = static_cast<int>(rooms.size()) + 1;
    rooms.push_back(Room{nextId, name});
    return nextId;
}

bool DataManager::addLesson(int teacherId, int subjectId, int classId, int periodsPerWeek) {
    if (!teacherExists(teacherId) || !subjectExists(subjectId) || !classExists(classId)) {
        return false;
    }
    lessons.push_back(Lesson{teacherId, subjectId, classId, periodsPerWeek});
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
