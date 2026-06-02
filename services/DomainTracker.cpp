#include "DomainTracker.h"
#include <iostream>
#include <stdexcept>

void DomainTracker::init(int numUnits, int numDays, int numPeriods,
                         const std::vector<LessonUnit>& units) {
    domains.resize(numUnits);

    for (int u = 0; u < numUnits; ++u) {
        domains[u].clear();
        int k = units[u].blockSize;
        for (int d = 0; d < numDays; ++d) {
            // A block of size k starting at period p needs periods up to p + k - 1
            // So p + k <= numPeriods
            for (int p = 0; p <= numPeriods - k; ++p) {
                domains[u].insert({d, p});
            }
        }
    }
}

void DomainTracker::initWithFixedEvents(int numUnits, int numDays, int numPeriods,
                                        const std::vector<LessonUnit>& units,
                                        const std::vector<FixedEvent>& fixedEvents) {
    // Start with standard domain initialization
    init(numUnits, numDays, numPeriods, units);

    // Now prune slots blocked by fixed events
    for (const auto& fe : fixedEvents) {
        // Map periodId (1-based) to period index (0-based)
        int blockedPeriodIdx = fe.periodId - 1;
        if (blockedPeriodIdx < 0 || blockedPeriodIdx >= numPeriods) continue;

        switch (fe.recurrence) {
            case RecurrenceType::DAILY: {
                // Block this period on ALL days
                for (int u = 0; u < numUnits; ++u) {
                    int k = units[u].blockSize;
                    auto it = domains[u].begin();
                    while (it != domains[u].end()) {
                        int startP = it->second;
                        // A block [startP, startP+k-1] overlaps blockedPeriodIdx
                        // if startP <= blockedPeriodIdx && startP + k - 1 >= blockedPeriodIdx
                        if (startP <= blockedPeriodIdx && startP + k - 1 >= blockedPeriodIdx) {
                            it = domains[u].erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                break;
            }
            case RecurrenceType::WEEKLY:
            case RecurrenceType::NONE: {
                // Block only specific (day, period)
                int blockedDayIdx = fe.dayId - 1; // 1-based to 0-based
                if (blockedDayIdx < 0 || blockedDayIdx >= numDays) continue;

                for (int u = 0; u < numUnits; ++u) {
                    int k = units[u].blockSize;
                    auto it = domains[u].begin();
                    while (it != domains[u].end()) {
                        if (it->first == blockedDayIdx) {
                            int startP = it->second;
                            if (startP <= blockedPeriodIdx && startP + k - 1 >= blockedPeriodIdx) {
                                it = domains[u].erase(it);
                                continue;
                            }
                        }
                        ++it;
                    }
                }
                break;
            }
            default:
                throw std::logic_error("Unknown recurrence type");
        }
    }
}

const std::set<std::pair<int,int>>& DomainTracker::getDomain(int unitIdx) const {
    return domains[unitIdx];
}

std::vector<std::set<std::pair<int,int>>> DomainTracker::getSnapshot() const {
    return domains;
}

void DomainTracker::restoreSnapshot(const std::vector<std::set<std::pair<int,int>>>& snapshot) {
    domains = snapshot;
}

bool DomainTracker::propagate(int placedUnitIdx, int d, int p,
                               const std::vector<LessonUnit>& units,
                               const std::vector<bool>& placed,
                               int numPeriods) {
    (void)numPeriods;
    const auto& placedUnit = units[placedUnitIdx];
    int k = placedUnit.blockSize;

    // Check all unplaced units to propagate overlap constraints
    for (size_t u = 0; u < units.size(); ++u) {
        if (placed[u] || static_cast<int>(u) == placedUnitIdx) {
            continue;
        }

        const auto& other = units[u];

        // Only conflicts if sharing class or teacher
        if (other.classId == placedUnit.classId || other.teacherId == placedUnit.teacherId) {
            // Find all domain slots on the same day d and filter out overlapping ones
            int k_other = other.blockSize;

            // We iterate over the other unit's domain on day d and remove overlapping slots
            auto it = domains[u].begin();
            while (it != domains[u].end()) {
                if (it->first == d) {
                    int p_other = it->second;
                    // Overlap check: C <= B and A <= D
                    // p_other <= p + k - 1 and p <= p_other + k_other - 1
                    if (p_other <= p + k - 1 && p_other + k_other - 1 >= p) {
                        it = domains[u].erase(it);
                        continue;
                    }
                }
                ++it;
            }

            // Fail-fast if this future unit has no valid starting slots left
            if (domains[u].empty()) {
                return false;
            }
        }
    }

    return true;
}
