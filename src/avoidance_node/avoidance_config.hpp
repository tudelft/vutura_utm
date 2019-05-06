#pragma once

#include <string>

#define AVOIDANCE_T_LOOKAHEAD 60.
#define AVOIDANCE_T_POP_TRAFFIC 20.
#define AVOIDANCE_RPZ 25.

class Avoidance_config {
public:
        Avoidance_config();
        int parse_config(std::string);

        double getTLookahead();
        double getTPopTraffic();
        double getRPZ();
    
private:

        double _t_lookahead;
        double _t_pop_traffic;
        double _r_pz;
};
