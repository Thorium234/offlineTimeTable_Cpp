#include "Menu.h"
#include <iostream>
#include <iomanip>
#include <string>

std::string Menu::readLineInput(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

int Menu::readIntInput(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        try {
            return std::stoi(line);
        } catch (...) {
            std::cout << "Invalid input. Please enter a number.\n";
        }
    }
}

void Menu::run() {
    while (true) {
        std::cout << "\n====================\n";
        std::cout << "TIMETABLE GENERATOR\n";
        std::cout << "====================\n";
        std::cout << "1. Add Teacher\n";
        std::cout << "2. Add Subject\n";
        std::cout << "3. Add Class\n";
        std::cout << "4. Add Room\n";
        std::cout << "5. Add Lesson\n";
        std::cout << "6. Generate Timetable\n";
        std::cout << "7. View Timetable\n";
        std::cout << "8. Exit\n";
        
        int choice = readIntInput("Select option: > ");
        
        switch (choice) {
            case 1:
                handleAddTeacher();
                break;
            case 2:
                handleAddSubject();
                break;
            case 3:
                handleAddClass();
                break;
            case 4:
                handleAddRoom();
                break;
            case 5:
                handleAddLesson();
                break;
            case 6:
                handleGenerateTimetable();
                break;
            case 7:
                handleViewTimetable();
                break;
            case 8:
                std::cout << "\nExiting Timetable Generator. Goodbye!\n";
                return;
            default:
                std::cout << "Invalid option. Please choose between 1 and 8.\n";
        }
    }
}

void Menu::handleAddTeacher() {
    std::cout << "\n--- Add Teacher ---\n";
    std::string name = readLineInput("Enter Teacher Name:\n> ");
    if (name.empty()) {
        std::cout << "Teacher name cannot be empty.\n";
        return;
    }
    int id = dm.addTeacher(name);
    std::cout << "Teacher \"" << name << "\" added with ID " << id << ".\n";
}

void Menu::handleAddSubject() {
    std::cout << "\n--- Add Subject ---\n";
    std::string name = readLineInput("Enter Subject Name:\n> ");
    if (name.empty()) {
        std::cout << "Subject name cannot be empty.\n";
        return;
    }
    int id = dm.addSubject(name);
    std::cout << "Subject \"" << name << "\" added with ID " << id << ".\n";
}

void Menu::handleAddClass() {
    std::cout << "\n--- Add Class ---\n";
    std::string name = readLineInput("Enter Class Name:\n> ");
    if (name.empty()) {
        std::cout << "Class name cannot be empty.\n";
        return;
    }
    int id = dm.addClass(name);
    std::cout << "Class \"" << name << "\" added with ID " << id << ".\n";
}

void Menu::handleAddRoom() {
    std::cout << "\n--- Add Room ---\n";
    std::string name = readLineInput("Enter Room Name:\n> ");
    if (name.empty()) {
        std::cout << "Room name cannot be empty.\n";
        return;
    }
    int id = dm.addRoom(name);
    std::cout << "Room \"" << name << "\" added with ID " << id << ".\n";
}

void Menu::handleAddLesson() {
    std::cout << "\n--- Add Lesson ---\n";
    
    // Check if we have teachers, subjects, and classes
    if (dm.teachers.empty()) {
        std::cout << "Cannot add lesson: No teachers registered yet.\n";
        return;
    }
    if (dm.subjects.empty()) {
        std::cout << "Cannot add lesson: No subjects registered yet.\n";
        return;
    }
    if (dm.classes.empty()) {
        std::cout << "Cannot add lesson: No classes registered yet.\n";
        return;
    }

    // 1. Show Teachers
    std::cout << "\nTeachers\n";
    for (const auto& t : dm.teachers) {
        std::cout << t.id << ". " << t.name << "\n";
    }
    int teacherId = readIntInput("Teacher ID:\n> ");
    if (!dm.teacherExists(teacherId)) {
        std::cout << "Invalid Teacher ID.\n";
        return;
    }

    // 2. Show Subjects
    std::cout << "\nSubjects\n";
    for (const auto& s : dm.subjects) {
        std::cout << s.id << ". " << s.name << "\n";
    }
    int subjectId = readIntInput("Subject ID:\n> ");
    if (!dm.subjectExists(subjectId)) {
        std::cout << "Invalid Subject ID.\n";
        return;
    }

    // 3. Show Classes
    std::cout << "\nClasses\n";
    for (const auto& c : dm.classes) {
        std::cout << c.id << ". " << c.name << "\n";
    }
    int classId = readIntInput("Class ID:\n> ");
    if (!dm.classExists(classId)) {
        std::cout << "Invalid Class ID.\n";
        return;
    }

    // 4. Input periods
    int periods = readIntInput("Periods Per Week:\n> ");
    if (periods <= 0 || periods > (DAYS * PERIODS)) {
        std::cout << "Periods per week must be between 1 and " << (DAYS * PERIODS) << ".\n";
        return;
    }

    if (dm.addLesson(teacherId, subjectId, classId, periods)) {
        std::cout << "Lesson created successfully.\n";
    } else {
        std::cout << "Failed to create lesson. Please verify your IDs.\n";
    }
}

void Menu::handleGenerateTimetable() {
    std::cout << "\nGenerating Timetable...\n";
    if (dm.lessons.empty()) {
        std::cout << "Cannot generate: No lessons have been assigned.\n";
        return;
    }
    timetable = engine.generate(dm);
    timetableGenerated = true;
    std::cout << "Timetable generated successfully!\n";
}

void Menu::handleViewTimetable() {
    if (!timetableGenerated) {
        std::cout << "\nPlease generate the timetable first (Option 6).\n";
        return;
    }
    timetable.print(dm);
}
