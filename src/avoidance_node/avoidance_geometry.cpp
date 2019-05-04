#include <fstream>

// json -- https://github.com/nlohmann/json
#include <nlohmann/json.hpp>

#include "avoidance_geometry.hpp"

Avoidance_geometry::Avoidance_geometry()
{

};

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
