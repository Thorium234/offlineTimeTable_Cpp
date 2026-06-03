#pragma once
#include <string>

class Teacher {
public:
    int id;
    std::string name;
    int maxConsecutive = 0; // 0 = no limit
};
