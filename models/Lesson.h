#pragma once

class Lesson {
public:
    int teacherId;
    int subjectId;
    int classId;
    int periodsPerWeek;
    int blockSize = 1; // 1 = single periods, 2 = doubles, 3 = triples, etc.
    int maxPerDay = 0; // 0 = no limit
};
