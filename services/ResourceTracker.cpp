#include "ResourceTracker.h"

ResourceTracker::ResourceTracker(int days, int periods)
    : totalDays(days), totalPeriods(periods) {}

void ResourceTracker::initResource(ResourceType type, int resourceId) {
    busyMap[type][resourceId] = std::vector<std::vector<int>>(totalDays, std::vector<int>(totalPeriods, 0));
}

bool ResourceTracker::isBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx) const {
    auto typeIt = busyMap.find(type);
    if (typeIt != busyMap.end()) {
        auto resIt = typeIt->second.find(resourceId);
        if (resIt != typeIt->second.end()) {
            if (dayIdx >= 0 && dayIdx < totalDays && periodIdx >= 0 && periodIdx < totalPeriods) {
                return resIt->second[dayIdx][periodIdx] != 0;
            }
        }
    }
    return false;
}

bool ResourceTracker::isBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx, int weekType) const {
    auto typeIt = busyMap.find(type);
    if (typeIt != busyMap.end()) {
        auto resIt = typeIt->second.find(resourceId);
        if (resIt != typeIt->second.end()) {
            if (dayIdx >= 0 && dayIdx < totalDays && periodIdx >= 0 && periodIdx < totalPeriods) {
                int mask = resIt->second[dayIdx][periodIdx];
                if (mask == 0) return false;
                // Every-week (bit 0) blocks everything
                if (mask & 1) return true;
                // Week-specific blocking
                if (weekType == 1 && (mask & 2)) return true;
                if (weekType == 2 && (mask & 4)) return true;
                return false;
            }
        }
    }
    return false;
}

void ResourceTracker::markBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx, bool busy) {
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
        if (busy) {
            busyMap[type][resourceId][dayIdx][periodIdx] |= 1;  // Set bit 0 (every week)
        } else {
            busyMap[type][resourceId][dayIdx][periodIdx] &= ~1; // Clear bit 0 only, preserve week-type bits
        }
    }
}

void ResourceTracker::markBusy(ResourceType type, int resourceId, int dayIdx, int periodIdx, int weekType) {
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
        int bit = (weekType == 0) ? 1 : (weekType == 1 ? 2 : 4);
        busyMap[type][resourceId][dayIdx][periodIdx] |= bit;
    }
}
