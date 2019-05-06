#ifndef AVOIDANCE_GEO_TOOLS_H
#define AVOIDANCE_GEO_TOOLS_H

struct position_params
{
    double latd;
    double lat;
    double lond;
    double lon;
    double alt;
    double coslat;
    double sinlat;
    double r;
};

void update_position_params(position_params& pos, double latd, double lond, double alt);

#endif // AVOIDANCE_GEO_TOOLS_H
