#pragma once
#include <string>
#include <vector>

class DataManager;
class Timetable;

struct ConflictInfo {
    enum class Type {
        TEACHER_BUSY,
        ROOM_BUSY,
        CLASS_BUSY,
        TEACHER_UNAVAILABLE,
        FIXED_EVENT,
        SELF_COLLISION
    };

    Type type;
    int conflictingResourceId;
    std::string description;
    int existingClassId = -1;
    int existingDay = -1;
    int existingPeriod = -1;
};

class ConflictChecker {
public:
    ConflictChecker(const DataManager &dataMgr, const Timetable &tbl);

    std::vector<ConflictInfo> checkPlacement(int classId, int subjectId,
                                              int teacherId, int roomId,
                                              int dayIndex, int periodIndex) const;

    std::vector<ConflictInfo> checkPlacementBlock(int classId, int subjectId,
                                                   int teacherId, int roomId,
                                                   int dayIndex, int periodIndex,
                                                   int blockSize) const;

    std::vector<ConflictInfo> checkMove(int srcClassId, int srcDay, int srcPeriod,
                                         int dstClassId, int dstDay, int dstPeriod) const;

    bool isPlacementValid(int classId, int subjectId, int teacherId, int roomId,
                           int dayIndex, int periodIndex) const;

    int conflictCount(int classId, int dayIndex, int periodIndex) const;

    static std::string typeToString(ConflictInfo::Type t);

private:
    const DataManager &dm;
    const Timetable &tt;
};
