#pragma once

#include <vector>

class Lesson {
public:
    int id = -1;
    int teacherId;
    int secondTeacherId = -1;
    int subjectId;
    int classId;
    std::vector<int> combinedClassIds;
    int periodsPerWeek;
    int blockSize = 1;
    int maxPerDay = 0;
    int weekType = 0;

    bool isEveryWeek() const { return weekType == 0; }
    bool isWeekA() const { return weekType == 0 || weekType == 1; }
    bool isWeekB() const { return weekType == 0 || weekType == 2; }
};
