#pragma once
#include "../models/LessonUnit.h"
#include "../models/FixedEvent.h"
#include <vector>
#include <set>
#include <utility>

class DomainTracker {
public:
    // Basic init (no fixed events)
    void init(int numUnits, int numDays, int numPeriods,
              const std::vector<LessonUnit>& units);

    // Init with fixed events: prunes blocked slots by recurrence type.
    // DAILY  -> blocks periodId on ALL days
    // WEEKLY -> blocks (dayId, periodId) only
    // NONE   -> blocks (dayId, periodId) only (single occurrence)
    void initWithFixedEvents(int numUnits, int numDays, int numPeriods,
                             const std::vector<LessonUnit>& units,
                             const std::vector<FixedEvent>& fixedEvents);

    const std::set<std::pair<int,int>>& getDomain(int unitIdx) const;

    // Get a copy of all current domains (for snapshot saving)
    std::vector<std::set<std::pair<int,int>>> getSnapshot() const;

    // Restore domains from a snapshot
    void restoreSnapshot(const std::vector<std::set<std::pair<int,int>>>& snapshot);

    // Propagate constraints when a unit is placed at starting period (d, p).
    // Returns false if any unplaced unit has an empty domain (fail-fast).
    bool propagate(int placedUnitIdx, int d, int p,
                   const std::vector<LessonUnit>& units,
                   const std::vector<bool>& placed,
                   int numPeriods);

private:
    std::vector<std::set<std::pair<int,int>>> domains;
};
