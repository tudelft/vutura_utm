#pragma once
#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "vutura_common/udp_source.hpp"
#include "vutura_common/requester.hpp"
#include "vutura_common/publisher.hpp"

#include "vutura_common.pb.h"

#include "clipper/clipper.hpp"

#include "avoidance_config.hpp"
#include "avoidance_geometry.hpp"
#include "avoidance_geo_tools.h"
#include "avoidance_intruder.hpp"

#define AVOIDANCE_KTS 0.514444

class AvoidanceNode {
public:
	AvoidanceNode(int instance, Avoidance_config& config, Avoidance_geometry& geometry);

        // initialisation
        int InitialiseSSD();

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
        void statebased_CD(Avoidance_intruder& inntruder);

	Requester _avoidance_req;
        Publisher _avoidance_pub;

	Avoidance_config& _avoidance_config;
        Avoidance_geometry& _avoidance_geometry;

        bool _gps_position_valid;
	double _lat;
	double _lon;
	double _alt;
        double _vn;
        double _ve;
        double _vd;

	bool _avoid;
        double _vn_sp;
        double _ve_sp;
        double _vd_sp;

        position_params _own_pos;

        // avoidance functions
        std::vector<Avoidance_intruder> _intruders;
        std::map<std::string, Avoidance_intruder*> _intr_inconf; // points to intruders that are in conflict
        std::map<std::string, Avoidance_intruder*> _intr_avoid; // points to intruders for which a avoidance maneuvre is in progress

        //SSD variables and functions
        signed long long Scale_to_clipper(double coord);
        double Scale_from_clipper(double coord);
        int ConstructSSD();
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
        } _SSD_v;
};
