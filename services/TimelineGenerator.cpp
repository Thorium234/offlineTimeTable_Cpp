#include "TimelineGenerator.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <map>

int TimelineGenerator::timeToMinutes(const std::string& time) {
    int h = 0, m = 0;
    char colon;
    std::istringstream iss(time);
    iss >> h >> colon >> m;
    return h * 60 + m;
}

std::string TimelineGenerator::minutesToTime(int minutes) {
    minutes = (minutes % 1440 + 1440) % 1440;
    int h = minutes / 60;
    int m = minutes % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << h << ":"
        << std::setw(2) << std::setfill('0') << m;
    return oss.str();
}

static bool isTimeInBreak(int timeMinutes, const BreakDefinition& brk) {
    int start = TimelineGenerator::timeToMinutes(brk.startTime);
    int end = TimelineGenerator::timeToMinutes(brk.endTime);
    return timeMinutes >= start && timeMinutes < end;
}

ScheduleConfig TimelineGenerator::generateFromConfig(
    const std::string& dayStart,
    const std::string& dayEnd,
    int defaultLessonDuration,
    const std::vector<Day>& days,
    const std::vector<BreakDefinition>& breaks,
    const std::vector<DayLayout>& specialLayouts
) {
    ScheduleConfig config;
    config.dayStart = dayStart;
    config.dayEnd = dayEnd;
    config.defaultLessonDurationMinutes = defaultLessonDuration;

    // Build a map of dayId -> special layout if any
    std::map<int, const DayLayout*> specialMap;
    for (const auto& sl : specialLayouts) {
        specialMap[sl.dayId] = &sl;
    }

    for (const auto& day : days) {
        DayLayout layout;
        layout.dayId = day.id;
        layout.dayName = day.name;

        auto sit = specialMap.find(day.id);
        if (sit != specialMap.end()) {
            layout.dayStart = sit->second->dayStart.empty() ? dayStart : sit->second->dayStart;
            layout.dayEnd = sit->second->dayEnd.empty() ? dayEnd : sit->second->dayEnd;
            layout.lessonDurationMinutes = sit->second->lessonDurationMinutes > 0
                ? sit->second->lessonDurationMinutes : defaultLessonDuration;
        } else {
            layout.dayStart = dayStart;
            layout.dayEnd = dayEnd;
            layout.lessonDurationMinutes = defaultLessonDuration;
        }

        int dayStartMin = timeToMinutes(layout.dayStart);
        int dayEndMin = timeToMinutes(layout.dayEnd);

        // Collect all break boundaries for this day
        std::vector<int> boundaries;
        boundaries.push_back(dayStartMin);
        boundaries.push_back(dayEndMin);

        for (const auto& brk : breaks) {
            if (std::find(brk.dayIds.begin(), brk.dayIds.end(), day.id) != brk.dayIds.end()) {
                int bs = timeToMinutes(brk.startTime);
                int be = timeToMinutes(brk.endTime);
                // Clamp to day boundaries
                bs = std::max(bs, dayStartMin);
                be = std::min(be, dayEndMin);
                if (bs < be) {
                    boundaries.push_back(bs);
                    boundaries.push_back(be);
                }
            }
        }

        std::sort(boundaries.begin(), boundaries.end());
        boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

        // Generate blocks: iterate through time intervals between boundaries
        int slotIdx = 0;
        for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
            int segStart = boundaries[i];
            int segEnd = boundaries[i + 1];
            if (segStart >= segEnd) continue;

            // Check if this segment is entirely within a break
            bool isBreak = false;
            for (const auto& brk : breaks) {
                if (std::find(brk.dayIds.begin(), brk.dayIds.end(), day.id) != brk.dayIds.end()) {
                    int bs = timeToMinutes(brk.startTime);
                    int be = timeToMinutes(brk.endTime);
                    if (segStart >= bs && segEnd <= be) {
                        TimeBlock block;
                        block.dayId = day.id;
                        block.slotIndex = slotIdx++;
                        block.startTime = minutesToTime(segStart);
                        block.endTime = minutesToTime(segEnd);
                        block.durationMinutes = segEnd - segStart;
                        block.type = BlockType::FIXED_BREAK;
                        block.label = brk.name;
                        layout.blocks.push_back(block);
                        isBreak = true;
                        break;
                    }
                }
            }
            if (isBreak) continue;

            // Slice remaining time into lesson slots
            int dur = layout.lessonDurationMinutes;
            int cursor = segStart;
            while (cursor + dur <= segEnd) {
                TimeBlock block;
                block.dayId = day.id;
                block.slotIndex = slotIdx++;
                block.startTime = minutesToTime(cursor);
                block.endTime = minutesToTime(cursor + dur);
                block.durationMinutes = dur;
                block.type = BlockType::LESSON_SLOT;
                block.label = "P" + std::to_string(block.slotIndex);
                layout.blocks.push_back(block);
                cursor += dur;
            }
        }

        config.dayLayouts.push_back(layout);
    }

    return config;
}

ScheduleConfig TimelineGenerator::generateUniform(
    const std::string& dayStart,
    const std::string& dayEnd,
    int lessonDurationMinutes,
    const std::vector<Day>& days,
    const std::vector<BreakDefinition>& breaks
) {
    return generateFromConfig(dayStart, dayEnd, lessonDurationMinutes, days, breaks, {});
}

std::vector<std::vector<Period>> TimelineGenerator::toPeriodsPerDay(const ScheduleConfig& config) {
    std::vector<std::vector<Period>> result;
    for (const auto& layout : config.dayLayouts) {
        std::vector<Period> dayPeriods;
        for (const auto& block : layout.blocks) {
            if (block.isLessonSlot()) {
                Period p;
                p.id = block.slotIndex;
                p.startTime = block.startTime;
                p.endTime = block.endTime;
                dayPeriods.push_back(p);
            }
        }
        result.push_back(dayPeriods);
    }
    return result;
}

int TimelineGenerator::countLessonSlots(const DayLayout& layout) {
    int count = 0;
    for (const auto& block : layout.blocks) {
        if (block.isLessonSlot()) ++count;
    }
    return count;
}
