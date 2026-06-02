#pragma once
#include <vector>
#include <string>
#include "../models/Teacher.h"
#include "../models/Subject.h"
#include "../models/SchoolClass.h"
#include "../models/Room.h"
#include "../models/Lesson.h"

class DataManager {
public:
    std::vector<Teacher> teachers;
    std::vector<Subject> subjects;
    std::vector<SchoolClass> classes;
    std::vector<Room> rooms;
    std::vector<Lesson> lessons;

    int addTeacher(const std::string& name);
    int addSubject(const std::string& name);
    int addClass(const std::string& name);
    int addRoom(const std::string& name);
    bool addLesson(int teacherId, int subjectId, int classId, int periodsPerWeek);

    std::string getTeacherName(int id) const;
    std::string getSubjectName(int id) const;
    std::string getClassName(int id) const;
    std::string getRoomName(int id) const;

    bool teacherExists(int id) const;
    bool subjectExists(int id) const;
    bool classExists(int id) const;
    bool roomExists(int id) const;
};
