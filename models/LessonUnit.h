#pragma once

#include <vector>

struct LessonUnit {
    int lessonIndex;
    int teacherId;
    int secondTeacherId = -1;
    int subjectId;
    int classId;
    std::vector<int> combinedClassIds;
    int reqRoomTypeId;
    int studentCount;
    int blockSize;
    int maxPerDay;
    int weekType = 0;
};
