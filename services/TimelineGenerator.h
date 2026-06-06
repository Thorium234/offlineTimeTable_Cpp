#pragma once
#include "../models/TimeBlock.h"
#include "../models/Day.h"
#include "../models/Period.h"
#include <string>
#include <vector>

class TimelineGenerator {
public:
    // Convert HH:MM to minutes from midnight
    static int timeToMinutes(const std::string& time);

    // Convert minutes to HH:MM string
    static std::string minutesToTime(int minutes);

    // Generate day layouts from configuration
    static ScheduleConfig generateFromConfig(
        const std::string& dayStart,
        const std::string& dayEnd,
        int defaultLessonDuration,
        const std::vector<Day>& days,
        const std::vector<BreakDefinition>& breaks,
        const std::vector<DayLayout>& specialLayouts
    );

    // Generate uniform layout (all days the same)
    static ScheduleConfig generateUniform(
        const std::string& dayStart,
        const std::string& dayEnd,
        int lessonDurationMinutes,
        const std::vector<Day>& days,
        const std::vector<BreakDefinition>& breaks
    );

    // Convert ScheduleConfig to Period objects per day
    static std::vector<std::vector<Period>> toPeriodsPerDay(const ScheduleConfig& config);

    // Get total lesson slots for a day
    static int countLessonSlots(const DayLayout& layout);
};
