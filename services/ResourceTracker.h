#pragma once
#include <vector>
#include <map>

enum class ResourceType {
    CLASS,
    TEACHER,
    ROOM
};

class ResourceTracker {
private:
    int totalDays;
    int totalPeriods;

    // Bitmask per (day, period): bit 0 = week 0 (every week), bit 1 = week A, bit 2 = week B
    std::map<ResourceType, std::map<int, std::vector<std::vector<int>>>> busyMap;

public:
    ResourceTracker(int days, int periods);

    void initResource(ResourceType type, int resourceId);

    // Legacy: checks if busy in any week
    bool isBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx) const;

    // Week-aware: weekType 0=every, 1=weekA, 2=weekB
    bool isBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx, int weekType) const;

    void markBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx, bool busy = true);

    void markBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx, int weekType);
};
