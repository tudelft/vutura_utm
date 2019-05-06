#pragma once
#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "vutura_common/udp_source.hpp"
#include "vutura_common/requester.hpp"

#include "vutura_common.pb.h"

#include "avoidance_config.hpp"
#include "avoidance_geometry.hpp"
#include "avoidance_geo_tools.h"
#include "avoidance_intruder.hpp"

class AvoidanceNode {
public:
	AvoidanceNode(int instance, Avoidance_config& config, Avoidance_geometry& geometry);

	int handle_periodic_timer();
	int handle_traffic(const TrafficInfo& traffic);
	int handle_gps_position(const GPSMessage& gps_info);
	int get_relative_coordinates(double lat_0, double lon_0, double lat_i, double lon_i, double *x, double *y);
    double getTimeStamp();

	static void periodic_timer_callback(EventSource *es);
	static void traffic_callback(EventSource *es);
	static void gps_position_callback(EventSource *es);
	static void avoidance_reply_callback(EventSource *es);

private:
    void traffic_housekeeping(double t_pop_traffic);

	Requester _avoidance_req;
	Avoidance_config& _avoidance_config;
    Avoidance_geometry& _avoidance_geometry;

	bool _gps_position_valid;
	double _lat;
	double _lon;
	double _alt;

	bool _avoid;
	double _vx_sp;
	double _vy_sp;
	double _vz_sp;

    position_params _own_pos;

    std::vector<Avoidance_intruder> _intruders;
};
