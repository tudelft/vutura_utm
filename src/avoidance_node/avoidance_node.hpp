#pragma once

#include <iostream>
#include <fstream>

#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"

#include "vutura_common.pb.h"

#include "nng_event_loop/event_loop.hpp"
#include "nng_event_loop/timer.hpp"
#include "nng_event_loop/subscriber.hpp"
#include "nng_event_loop/publisher.hpp"

#include "avoidance_config.hpp"
#include "avoidance_geometry.hpp"
#include "avoidance_geo_tools.h"
#include "avoidance_intruder.hpp"

#define AVOIDANCE_KTS 0.514444

class AvoidanceNode : public EventLoop {
public:
	AvoidanceNode(int instance, Avoidance_config& config, Avoidance_geometry& geometry);

	void init();
	// initialisation
	int InitialiseSSD();
	int InitialiseLogger();

	int handle_periodic_timer();
	void traffic_callback(std::string message);
	int handle_traffic(const TrafficInfo& traffic);
	void gps_position_callback(std::string message);
	int handle_gps_position(const GPSMessage& gps_info);
	int get_relative_coordinates(double lat_0, double lon_0, double lat_i, double lon_i, double *x, double *y);
	double getTimeStamp();

//	static void avoidance_reply_callback(EventSource *es);

private:
	void write_log();
	void write_traffic_log(std::string acid, double latd, double lond, double alt, double hdg, double gs, double recorded_time, double delay);
	void traffic_housekeeping(double t_pop_traffic);
	void statebased_CD(Avoidance_intruder& inntruder);

	//Requester _avoidance_req;
	Timer _periodic_timer;
	Subscriber _traffic_sub;
	Subscriber _gps_sub;
	Publisher _avoidance_pub;

	Avoidance_config& _avoidance_config;
	Avoidance_geometry& _avoidance_geometry;

	int _instance;
	bool _gps_position_valid;
	double _time_gps;
	double _lat;
	double _lon;
	double _alt;
	double _vn;
	double _ve;
	double _vd;
	bool _target_wp_available;
	uint32_t _target_wp;
	double _wind_north;
	double _wind_east;
	double _time_to_target;
	double _t_lookahead;

	bool _avoid;
	bool _in_pz;
	double _vn_sp;
	double _ve_sp;
	double _vd_sp;
	double _latd_sp;
	double _lond_sp;

	bool _logging;
	std::ofstream _logfile;
	std::ofstream _traffic_logfile;

	position_params _own_pos;

	// avoidance functions
	std::vector<Avoidance_intruder> _intruders;
	std::map<std::string, Avoidance_intruder*> _intr_inconf; // points to intruders that are in conflict
	std::map<std::string, Avoidance_intruder*> _intr_avoid; // points to intruders for which a avoidance maneuvre is in progress

	//SSD variables and functions
	int ConstructSSD();
	ClipperLib::Paths ConstructGeofencePolygons(Avoidance_intruder& intruder);
	int SSDResolution();
	int ResumeNav();

	// To be initialised by the initialisation function
	struct SSD_config
	{
		double hsepm;
		double alpham;
		double vmin;
		double vmax;
		double vset;
		ClipperLib::Path v_c_out;
		ClipperLib::Path v_c_in;
		ClipperLib::Path v_c_set_out;
		ClipperLib::Path v_c_set_in;
	} _SSD_c;

	struct SSD_var
	{
		ClipperLib::Paths ARV_scaled;
		ClipperLib::Paths ARV_scaled_speed;
		ClipperLib::Paths ARV_scaled_geofence;
	} _SSD_v;
};
