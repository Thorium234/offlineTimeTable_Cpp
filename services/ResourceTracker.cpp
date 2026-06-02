#include "ResourceTracker.h"

ResourceTracker::ResourceTracker(int days, int periods)
    : totalDays(days), totalPeriods(periods) {}

void ResourceTracker::initResource(ResourceType type, int resourceId) {
    busyMap[type][resourceId] = std::vector<std::vector<bool>>(totalDays, std::vector<bool>(totalPeriods, false));
}

bool ResourceTracker::isBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx) const {
    auto typeIt = busyMap.find(type);
    if (typeIt != busyMap.end()) {
        auto resIt = typeIt->second.find(resourceId);
        if (resIt != typeIt->second.end()) {
            if (dayIdx >= 0 && dayIdx < totalDays && periodIdx >= 0 && periodIdx < totalPeriods) {
                return resIt->second[dayIdx][periodIdx];
            }
        }
    }
    return false;
}

void ResourceTracker::markBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx, bool busy) {
    // If not found in map, initialize it
    auto typeIt = busyMap.find(type);
    if (typeIt == busyMap.end()) {
        initResource(type, resourceId);
    } else {
        auto resIt = typeIt->second.find(resourceId);
        if (resIt == typeIt->second.end()) {
            initResource(type, resourceId);
        }
    }
    if (dayIdx >= 0 && dayIdx < totalDays && periodIdx >= 0 && periodIdx < totalPeriods) {
        busyMap[type][resourceId][dayIdx][periodIdx] = busy;
    }
}
