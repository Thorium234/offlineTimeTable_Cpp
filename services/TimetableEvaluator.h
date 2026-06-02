#pragma once
#include "../timetable/Timetable.h"
#include "DataManager.h"

struct EvaluationWeights {
    int teacherGapWeight = 10;
    int classGapWeight = 15;
    int subjectClusterWeight = 5;
    int unscheduledWeight = 50;
    int teacherPreferredBonus = 10;
    int teacherUndesirablePenalty = 20;
};

class TimetableEvaluator {
public:
    EvaluationWeights weights;
    int calculateScore(const Timetable& timetable, const DataManager& dm) const;
};
