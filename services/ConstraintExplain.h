#pragma once
#include <string>
#include "../models/Teacher.h"
#include "../models/Subject.h"
#include "../models/SchoolClass.h"
#include "../models/Room.h"
#include "../models/Day.h"
#include "../models/Period.h"
#include "../models/ConstraintViolation.h"

class ConstraintExplain {
public:
    static ConstraintViolation explainTeacherUnavailable(int teacherId, const std::string& teacherName, int dayId, const std::string& dayName, int periodId, const std::string& periodTime);
    static ConstraintViolation explainRoomCapacity(int roomId, const std::string& roomName, int requiredCapacity, int availableCapacity);
    static ConstraintViolation explainFixedEvent(int eventId, const std::string& eventName, int dayId, const std::string& dayName, int periodId, const std::string& periodTime);
    // Add more explanations as needed
};
