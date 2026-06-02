# Timetable Generator

A simple, lightweight C++ Timetable Generator implementing a constraint-based scheduling engine for schools.

## Features & Constraints

The system is designed with a minimal but robust core to generate conflict-free schedules satisfying the following rules:
1. **Class Constraint**: A school class cannot have two lessons scheduled in the same time slot.
2. **Teacher Constraint**: A teacher cannot teach two different classes in the same time slot.
3. **Period Completion**: The required periods per week for each assigned lesson must be satisfied where resources allow.

## Directory Structure

```text
TimetableGenerator/
├── main.cpp                # App entry point
├── Makefile                # Build system (g++)
├── ReadMe.md               # Documentation
├── models/                 # Domain objects
│   ├── Teacher.h
│   ├── Subject.h
│   ├── SchoolClass.h
│   ├── Room.h
│   └── Lesson.h
├── services/               # System logic
│   ├── DataManager.h       # In-memory storage of users and lessons
│   ├── DataManager.cpp
│   ├── TimetableEngine.h   # Scheduling algorithms
│   └── TimetableEngine.cpp
├── timetable/              # Grid models & presentation
│   ├── Timetable.h
│   └── Timetable.cpp
└── utils/                  # Command-line interface utilities
    ├── Menu.h
    └── Menu.cpp
```

## Compilation

Build the application with your terminal using standard `g++` compilation:

```bash
make
```

To clean build artifacts:

```bash
make clean
```

## How to Run

After compilation, launch the CLI using:

```bash
./timetableGen
```

### CLI Workflow Example

1. **Add Teacher**: input teacher's name (e.g. "John Maina").
2. **Add Subject**: input subject's name (e.g. "Mathematics").
3. **Add Class**: input class name (e.g. "Form 1A").
4. **Add Room**: input classroom/lab name (e.g. "Lab 1").
5. **Add Lesson**: link a teacher, subject, class, and input requested periods per week.
6. **Generate Timetable**: triggers the scheduling algorithm.
7. **View Timetable**: outputs clean scheduling grids formatted by class to the CLI.
8. **Exit**: shuts down the program.
