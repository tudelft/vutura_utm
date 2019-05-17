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

struct latdlond
{
	double latd;
	double lond;
};

void update_position_params(position_params& pos, double latd, double lond, double alt);
void calc_position_from_reference(position_params& reference_pos, latdlond& target_latdlond, double pn, double pe);

#endif // AVOIDANCE_GEO_TOOLS_H
