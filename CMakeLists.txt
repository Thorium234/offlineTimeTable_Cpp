cmake_minimum_required(VERSION 3.12)
project(timetableGen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic compilation of Qt MOC, UIC and RCC files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find Qt5 packages
find_package(Qt5 COMPONENTS Widgets Sql REQUIRED)

# Common sources used by both CLI and GUI
set(COMMON_SRCS
    services/DataManager.cpp
    services/SQLiteService.cpp
    services/TimetableEngine.cpp
    services/ResourceTracker.cpp
    services/ExportService.cpp
    services/TimetableEvaluator.cpp
    services/FeasibilityChecker.cpp
    services/DomainTracker.cpp
    services/Benchmark.cpp
    services/BacktrackingSolver.cpp
    services/GreedySolver.cpp
    timetable/Timetable.cpp
    services/AnalyticsService.cpp
)

# Unified target - aSc TimeTables style layout
add_executable(timetableGen
    main.cpp
    utils/Menu.cpp
    gui/MainWindow.cpp
    gui/ribbon/RibbonToolbar.cpp
    gui/sidebar/DataSidebar.cpp
    gui/timetableview/TimetableViewWidget.cpp
    gui/teachers/TeacherDialog.cpp
    gui/subjects/SubjectDialog.cpp
    gui/classes/ClassDialog.cpp
    gui/rooms/RoomDialog.cpp
    ${COMMON_SRCS}
)
target_link_libraries(timetableGen Qt5::Widgets Qt5::Sql -static-libstdc++ -static-libgcc)

# Test Runner target
add_executable(test_runner
    tests/test_runner.cpp
    ${COMMON_SRCS}
)
target_link_libraries(test_runner Qt5::Widgets Qt5::Sql -static-libstdc++ -static-libgcc)
