CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = timetableGen
SRCS = main.cpp \
       services/DataManager.cpp \
       services/TimetableEngine.cpp \
       timetable/Timetable.cpp \
       utils/Menu.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
