#pragma once
#include <string>

class SchoolClass {
public:
    int id;
    std::string name;
    int studentCount;
    
    // Education block - earliest and latest period for this class (0-based, -1 = no limit)
    int earliestPeriod = -1;  // -1 means no earliest period constraint
    int latestPeriod = -1;   // -1 means no latest period constraint
    
    // Class division/group support
    int divisionId = -1;     // -1 means not part of a division
    int groupId = -1;        // -1 means not part of a group
};
