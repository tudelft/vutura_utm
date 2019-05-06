#pragma once

#include <string>

#define AVOIDANCE_T_LOOKAHEAD 60.
#define AVOIDANCE_T_POP_TRAFFIC 20.

class Avoidance_config {
public:
    Avoidance_config();
    int parse_config(std::string);

    double getTPopTraffic();
    
private:

    double _t_lookahead;
    double _t_pop_traffic;
};
