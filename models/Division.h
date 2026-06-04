#pragma once
#include <string>
#include <vector>

class Division {
public:
    int id;
    std::string name;
    std::vector<int> classIds;  // List of class IDs in this division
    bool canRunInParallel;      // Can classes in this division run different lessons simultaneously
};
