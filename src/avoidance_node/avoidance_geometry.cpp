#include <fstream>
#include <iostream>

// json -- https://github.com/nlohmann/json
#include <nlohmann/json.hpp>

#include "avoidance_geometry.hpp"

Avoidance_geometry::Avoidance_geometry() :
	_geofence(),
	_flightplan(),
	_airspeed(0),
	_altitude(0)
{

}

int Avoidance_geometry::parse_geometry(std::string geometry_file)
{
	nlohmann::json geometry;
	try {
		std::ifstream i(geometry_file);
		i >> geometry;
		for(unsigned i = 0; i < geometry["features"].size(); i++)
		{
			nlohmann::json feature = geometry["features"].at(i);
			try {
				if (feature["properties"]["name"] == "geofence")
				{
					std::vector<std::vector<double>> geofence = feature["geometry"]["coordinates"][0];
					_geofence = geofence;
				}
				if (feature["properties"]["name"] == "flightplan")
				{
					std::vector<std::vector<double>> flightplan = feature["geometry"]["coordinates"];
					_flightplan = flightplan;
					try {
						_airspeed = feature["properties"]["speed"];
						_altitude = feature["properties"]["altitude"];
					} catch (...) {
						std::cerr << "No airspeed or altitude defined in flighplan section of geometry file" << std::endl;
						return -1;
					}
				}
			} catch (...) {
				std::cerr << "Invalid geometry feature imported from geojson (name not specified)" << std::endl;
				return -1;
			}
		}
	} catch (...) {
		std::cerr << "Failed to parse geometry file: " << geometry_file << std::endl;
		return -1;
	}
	std::cout << geometry.dump(4) << std::endl;
	return 0;
}

struct latdlond Avoidance_geometry::getWpCoordinatesLatLon(size_t wp_index)
{
	latdlond wp_coord;
	wp_coord.latd = _flightplan.at(wp_index)[1];
	wp_coord.lond = _flightplan.at(wp_index)[0];
	return wp_coord;
}

struct n_e_coordinate Avoidance_geometry::getRelWpNorthEast(position_params &pos, size_t wp_index)
{
	latdlond wp_latdlond = getWpCoordinatesLatLon(wp_index);
	n_e_coordinate rel_waypoint_ne = calc_northeast_from_reference(pos, wp_latdlond);

	return rel_waypoint_ne;
}

std::vector<std::vector<double>> Avoidance_geometry::getGeofenceLatdLond()
{
	return _geofence;
}
