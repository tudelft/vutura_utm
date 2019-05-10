#pragma once

#include <string>

#define AVOIDANCE_T_LOOKAHEAD 60.
#define AVOIDANCE_T_POP_TRAFFIC 20.
#define AVOIDANCE_RPZ 25.
#define AVOIDANCE_RPZ_MAR 0.5
#define AVOIDANCE_N_ANGLE 180
#define AVOIDANCE_V_MIN 5.0
#define AVOIDANCE_V_MAX 20.0
#define AVOIDANCE_V_SET 12.0

class Avoidance_config {
public:
        Avoidance_config();
        int parse_config(std::string);

        double getTLookahead();
        double getTPopTraffic();
        double getRPZ();
        double getRPZMar();
        unsigned getNAngle();
        double getVMin();
        double getVMax();
        double getVSet();
    
private:

        double _t_lookahead;
        double _t_pop_traffic;
        double _r_pz;
        double _r_pz_mar;
        unsigned _n_angle;
        double _v_min;
        double _v_max;
        double _v_set;
};
