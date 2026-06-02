#pragma once
#include <string>
#include <stdexcept>

// Recurrence semantics for FixedEvents.
// NONE    = single occurrence on a specific (day, period)
// DAILY   = blocks the same period on every school day
// WEEKLY  = blocks a specific (day, period) every week
// Architecture note: avoid assuming DAILY == eternal; keep extensible for
// startDate/endDate support in a later sprint.
enum class RecurrenceType {
    NONE   = 0,
    DAILY  = 1,
    WEEKLY = 2
};

// Returns human-readable label for export / display.
inline std::string recurrenceToString(RecurrenceType r) {
    switch (r) {
        case RecurrenceType::NONE:   return "NONE";
        case RecurrenceType::DAILY:  return "DAILY";
        case RecurrenceType::WEEKLY: return "WEEKLY";
        default:
            throw std::logic_error("Unknown recurrence type");
    }
}

struct FixedEvent {
    int id;
    int dayId;    // Ignored when recurrence == DAILY (all days are blocked)
    int periodId; // Always used
    std::string name;
    RecurrenceType recurrence = RecurrenceType::NONE;
};
