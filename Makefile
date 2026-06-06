CXX      := g++
CXXFLAGS := -std=c++17 -fPIC -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor \
            -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual \
            -Wconversion -Wsign-conversion -Wnull-dereference \
            -Wformat=2 -Wmisleading-indentation -Wduplicated-cond \
            $(shell pkg-config --cflags Qt5Widgets Qt5Sql Qt5PrintSupport)

LDFLAGS  := $(shell pkg-config --libs Qt5Widgets Qt5Sql Qt5PrintSupport)

# Release by default; override: make BUILD=debug
BUILD    ?= release
ifeq ($(BUILD),debug)
    CXXFLAGS += -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer -Werror
    LDFLAGS  += -fsanitize=address,undefined
else
    CXXFLAGS += -O2 -DNDEBUG
endif

TARGET   := timetableGen
TEST_TARGET := test_runner

# All sources
SRCS := main.cpp \
        gui/MainWindow.cpp \
        gui/ribbon/RibbonToolbar.cpp \
        gui/sidebar/DataSidebar.cpp \
        gui/timetableview/TimetableViewWidget.cpp \
        gui/timetableview/TimetableScene.cpp \
        gui/timetableview/LessonCardItem.cpp \
        gui/dashboard/DashboardWidget.cpp \
        gui/teachers/TeacherDialog.cpp \
        gui/subjects/SubjectDialog.cpp \
        gui/classes/ClassDialog.cpp \
        gui/rooms/RoomDialog.cpp \
        services/DataManager.cpp \
        services/SQLiteService.cpp \
        services/TimetableEngine.cpp \
        services/ResourceTracker.cpp \
        services/ExportService.cpp \
        services/TimetableEvaluator.cpp \
        services/FeasibilityChecker.cpp \
        services/DomainTracker.cpp \
        services/Benchmark.cpp \
        services/BacktrackingSolver.cpp \
        services/GreedySolver.cpp \
        services/PdfReportService.cpp \
        services/ConstraintExplanationService.cpp \
        services/ConstraintExplain.cpp \
        services/ConflictChecker.cpp \
        services/UndoRedoStack.cpp \
        timetable/Timetable.cpp \
        services/AnalyticsService.cpp

OBJS := $(SRCS:.cpp=.o)

TEST_SRCS := tests/test_runner.cpp \
             $(filter-out main.cpp, $(SRCS))
TEST_OBJS := $(TEST_SRCS:.cpp=.o)

.PHONY: all clean test run format

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET)

# Requires clang-format
format:
	@which clang-format > /dev/null 2>&1 || { echo "clang-format not found"; exit 1; }
	find . -name '*.cpp' -o -name '*.h' | grep -v build_cmake | xargs clang-format -i -style=file
