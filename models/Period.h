#pragma once
#include <string>

class Period {
public:
    int id;
    std::string label;     // display label e.g. "Period 1"
    std::string startTime;
    std::string endTime;
};
