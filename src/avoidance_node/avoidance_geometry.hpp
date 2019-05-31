#pragma once

#include <string>
#include <vector>
#include "avoidance_geo_tools.h"

class Avoidance_geometry {
public:
	Avoidance_geometry();
	int parse_geometry(std::string);

	struct latdlond getWpCoordinatesLatLon(size_t wp_index);
	struct n_e_coordinate getRelWpNorthEast(position_params& pos, size_t wp_index);
	std::vector<std::vector<double>> getGeofenceLatdLond();
	std::vector<std::vector<double>> getSoftGeofenceLatdLond();

private:
	std::vector<std::vector<double>> _geofence;
	std::vector<std::vector<double>> _flightplan;
	std::vector<std::vector<double>> _soft_geofence;

	double _airspeed;
	double _altitude;
};
