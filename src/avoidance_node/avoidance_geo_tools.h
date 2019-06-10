#ifndef AVOIDANCE_GEO_TOOLS_H
#define AVOIDANCE_GEO_TOOLS_H

#include <array>
#include "clipper/clipper.hpp"

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

struct geofence_sector
{
	std::array<n_e_coordinate,2> north_east;
	n_e_coordinate a;
	n_e_coordinate n;
	n_e_coordinate distv;
	double dist;
	double rotation;
};

void update_position_params(position_params& pos, double latd, double lond, double alt);
latdlond calc_latdlond_from_reference(position_params& reference_pos, n_e_coordinate& target_ne);
n_e_coordinate calc_northeast_from_reference(position_params& reference_pos, latdlond& target_latdlond);
geofence_sector calc_geofence_sector_from_n_e(std::array<n_e_coordinate,2>& n_e);
double calc_distance_from_reference_and_target_latdlond(position_params& reference_pos, latdlond& target_latdlond);

// clipper tools
signed long long Scale_to_clipper(double coord);
double Scale_from_clipper(double coord);

// area check using clipper
bool latdlond_inside_geofence(position_params& reference_pos, std::vector<std::vector<double>>& geofence_ll, latdlond& target_latdlond);
bool latdlond_inside_clipperpath(position_params& reference_pos, latdlond& target_latdlond, ClipperLib::Path path);

// airspeed / groundspeed conversion
double calc_groundspeed_at_hdg(double gs_n, double gs_e, double wind_n, double wind_e, double hdgd);

#endif // AVOIDANCE_GEO_TOOLS_H
