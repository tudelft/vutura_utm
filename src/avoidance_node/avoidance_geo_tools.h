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

struct n_e_coordinate
{
	double north;
	double east;
};

void update_position_params(position_params& pos, double latd, double lond, double alt);
latdlond calc_latdlond_from_reference(position_params& reference_pos, n_e_coordinate& target_ne);
n_e_coordinate calc_northeast_from_reference(position_params& reference_pos, latdlond& target_latdlond);


#endif // AVOIDANCE_GEO_TOOLS_H
