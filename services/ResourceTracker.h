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

    std::map<ResourceType, std::map<int, std::vector<std::vector<bool>>>> busyMap;

public:
    ResourceTracker(int days, int periods);

    void initResource(ResourceType type, int resourceId);

    bool isBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx) const;

    void markBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx, bool busy = true);
};
