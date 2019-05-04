#pragma once

#include <string>
#include <vector>

class Avoidance_geometry {
public:
    Avoidance_geometry();
    int parse_geometry(std::string);

private:
    std::vector<std::vector<double>> _geofence;
    std::vector<std::vector<double>> _flightplan;

    double _airspeed;
    double _altitude;
};