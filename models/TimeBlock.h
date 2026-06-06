#pragma once
#include <string>
#include <vector>

enum class BlockType { LESSON_SLOT, FIXED_BREAK };

struct TimeBlock {
    int dayId;
    int slotIndex;
    std::string startTime;
    std::string endTime;
    int durationMinutes;
    BlockType type;
    std::string label;

    bool isBreak() const { return type == BlockType::FIXED_BREAK; }
    bool isLessonSlot() const { return type == BlockType::LESSON_SLOT; }
};

struct BreakDefinition {
    std::string name;
    std::string startTime;
    std::string endTime;
    std::vector<int> dayIds;
};

struct DayLayout {
    int dayId;
    std::string dayName;
    std::string dayStart;
    std::string dayEnd;
    int lessonDurationMinutes;
    std::vector<TimeBlock> blocks;
};

struct ScheduleConfig {
    std::string dayStart;
    std::string dayEnd;
    int defaultLessonDurationMinutes;
    std::vector<DayLayout> dayLayouts;
};
