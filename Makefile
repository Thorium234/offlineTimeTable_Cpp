CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = timetableGen
SRCS = main.cpp \
       services/DataManager.cpp \
       services/TimetableEngine.cpp \
       services/ResourceTracker.cpp \
       services/ExportService.cpp \
       services/TimetableEvaluator.cpp \
       services/FeasibilityChecker.cpp \
       services/DomainTracker.cpp \
       services/Benchmark.cpp \
       services/BacktrackingSolver.cpp \
       services/GreedySolver.cpp \
       timetable/Timetable.cpp \
       utils/Menu.cpp \
       services/AnalyticsService.cpp
OBJS = $(SRCS:.cpp=.o)

TEST_TARGET = test_runner
TEST_SRCS = tests/test_runner.cpp \
            services/DataManager.cpp \
            services/TimetableEngine.cpp \
            services/ResourceTracker.cpp \
            services/ExportService.cpp \
            services/TimetableEvaluator.cpp \
            services/FeasibilityChecker.cpp \
            services/DomainTracker.cpp \
            services/Benchmark.cpp \
            services/BacktrackingSolver.cpp \
            services/GreedySolver.cpp \
            timetable/Timetable.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

$(TEST_TARGET): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_OBJS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OBJS) $(TEST_TARGET)

.PHONY: all clean test
