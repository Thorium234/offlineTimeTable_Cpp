cmake_minimum_required(VERSION 3.16)
project(timetableGen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── Build type ────────────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ── Compiler warnings (applied globally) ──────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
    add_compile_options(-Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual)
    add_compile_options(-Wconversion -Wsign-conversion -Wnull-dereference)
    add_compile_options(-Wformat=2 -Wmisleading-indentation -Wduplicated-cond)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # add_compile_options(-Werror)  # warnings = errors in debug builds
    endif()
endif()

# ── Sanitizers (Debug builds) ─────────────────────────────────────────────────
option(ENABLE_ASAN "Enable AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
if(ENABLE_ASAN OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

find_package(Qt5 COMPONENTS Widgets Sql PrintSupport REQUIRED)

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
    services/PdfReportService.cpp
    services/ConstraintExplanationService.cpp
    services/ConstraintExplain.cpp
    services/ConflictChecker.cpp
    services/UndoRedoStack.cpp
    services/TimelineGenerator.cpp
)

# Main timetable generator binary (GUI + headless solver backend)
add_executable(timetableGen
    main.cpp
    gui/MainWindow.cpp
    gui/ribbon/RibbonToolbar.cpp
    gui/sidebar/DataSidebar.cpp
    gui/timetableview/TimetableViewWidget.cpp
    gui/timetableview/TimetableScene.cpp
    gui/timetableview/LessonCardItem.cpp
    gui/dashboard/DashboardWidget.cpp
    gui/teachers/TeacherDialog.cpp
    gui/subjects/SubjectDialog.cpp
    gui/classes/ClassDialog.cpp
    gui/rooms/RoomDialog.cpp
    gui/substitutions/SubstitutionWidget.cpp
    gui/divisions/DivisionWidget.cpp
    gui/constraints/ConstraintRelaxationDialog.cpp
    ${COMMON_SRCS}
)

target_link_libraries(timetableGen
    Qt5::Widgets
    Qt5::Sql
    Qt5::PrintSupport
)

# Test runner
add_executable(test_runner
    tests/test_runner.cpp
    ${COMMON_SRCS}
)

target_link_libraries(test_runner
    Qt5::Widgets
    Qt5::Sql
    Qt5::PrintSupport
)

# ── Custom targets ────────────────────────────────────────────────────────────
add_custom_target(run
    COMMAND $<TARGET_FILE:timetableGen>
    DEPENDS timetableGen
    USES_TERMINAL
)

add_custom_target(check
    COMMAND $<TARGET_FILE:test_runner>
    DEPENDS test_runner
    USES_TERMINAL
)
