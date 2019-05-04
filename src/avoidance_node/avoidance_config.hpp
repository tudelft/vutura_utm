#pragma once

#include <string>

#define AVOIDANCE_T_LOOKAHEAD 60.

class Avoidance_config {
public:
    Avoidance_config();
    int parse_config(std::string);
    
private:

    double _t_lookahead;
};