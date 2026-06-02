#include "ConstraintExplain.h"
#include "DataManager.h"
#include <sstream>

ConstraintViolation ConstraintExplain::explainTeacherUnavailable(int teacherId, const std::string& teacherName,
                                                            int dayId, const std::string& dayName,
                                                            int periodId, const std::string& periodTime) {
    ConstraintViolation cv;
    cv.type = "TeacherUnavailable";
    std::ostringstream oss;
    oss << "Teacher " << teacherName << " (ID " << teacherId << ") is unavailable on "
        << dayName << " (Day ID " << dayId << ") during period " << periodTime << " (Period ID " << periodId << ")";
    cv.explanation = oss.str();
    return cv;
}

ConstraintViolation ConstraintExplain::explainRoomCapacity(int roomId, const std::string& roomName,
                                                          int requiredCapacity, int availableCapacity) {
    ConstraintViolation cv;
    cv.type = "RoomCapacity";
    std::ostringstream oss;
    oss << "Room " << roomName << " (ID " << roomId << ") cannot accommodate required capacity "
        << requiredCapacity << ". Available capacity is " << availableCapacity << ".";
    cv.explanation = oss.str();
    return cv;
}

ConstraintViolation ConstraintExplain::explainFixedEvent(int eventId, const std::string& eventName,
                                                         int dayId, const std::string& dayName,
                                                         int periodId, const std::string& periodTime) {
    ConstraintViolation cv;
    cv.type = "FixedEventConflict";
    std::ostringstream oss;
    oss << "Fixed event '" << eventName << "' (ID " << eventId << ") blocks "
        << dayName << " (Day ID " << dayId << ") period " << periodTime << " (Period ID " << periodId << ")";
    cv.explanation = oss.str();
    return cv;
}
