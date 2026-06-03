#include "ConstraintExplanationService.h"
#include "../timetable/Timetable.h"
#include "DataManager.h"

std::vector<ConstraintViolation> ConstraintExplanationService::explain(const Timetable& /*timetable*/, const DataManager& dm) const {
    std::vector<ConstraintViolation> violations;
    const auto& log = dm.getPlacementRejectLog();
    for (const auto& msg : log) {
        ConstraintViolation v;
        v.type = "Placement Rejection";
        v.explanation = msg;
        v.recommendation = "Review the constraint causing this rejection and adjust data accordingly.";
        violations.push_back(std::move(v));
    }
    return violations;
}
