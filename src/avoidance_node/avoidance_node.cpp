#include <unistd.h>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <array>
#include "sys/time.h"

#include "vutura_common.pb.h"

#include "avoidance_node.hpp"

using namespace std::placeholders;

AvoidanceNode::AvoidanceNode(int instance, Avoidance_config& config, Avoidance_geometry& geometry) :
	//_avoidance_req(this, socket_name(SOCK_REQREP_AVOIDANCE_COMMAND, instance), avoidance_reply_callback),
	_instance(instance),
	_periodic_timer(this),
	_traffic_sub(this),
	_gps_sub(this),
	_avoidance_pub(socket_name(SOCK_REQREP_AVOIDANCE_COMMAND, instance)),
	_avoidance_config(config),
	_avoidance_geometry(geometry),
	_gps_position_valid(false),
	_time_gps(0),
	_lat(0),
	_lon(0),
	_alt(0),
	_vn(0),
	_ve(0),
	_vd(0),
	_target_wp_available(0),
	_target_wp(0),
	_time_to_target(0),
	_t_lookahead(0),
	_avoid(false),
	_in_pz(false),
	_vn_sp(0),
	_ve_sp(0),
	_vd_sp(0),
	_latd_sp(0),
	_lond_sp(0),
	_logging(false),
	_intruders()
{

}

void AvoidanceNode::init()
{
	InitialiseSSD();
	InitialiseLogger();

	_periodic_timer.set_timeout_callback(std::bind(&AvoidanceNode::handle_periodic_timer, this));
	_periodic_timer.start_periodic(200);

	_traffic_sub.set_receive_callback(std::bind(&AvoidanceNode::traffic_callback, this, _1));
	//_traffic_sub.subscribe(socket_name(SOCK_PUBSUB_TRAFFIC_INFO, _instance));
	_traffic_sub.subscribe(SOCK_PUBSUB_TRAFFIC_INFO);

	_gps_sub.set_receive_callback(std::bind(&AvoidanceNode::gps_position_callback, this, _1));
	_gps_sub.subscribe(socket_name(SOCK_PUBSUB_GPS_POSITION, _instance));
}

int AvoidanceNode::InitialiseSSD()
{
	// Import configuration and copy relevant settings for SSD
	const unsigned N_angle		= _avoidance_config.getNAngle();
	const double hsep		= _avoidance_config.getRPZ();
	const double margin		= _avoidance_config.getRPZMar();
	_SSD_c.hsepm			= hsep * margin;
	_SSD_c.alpham			= 0.4999 * M_PI;
	_SSD_c.vmin			= std::max(0.1, _avoidance_config.getVMin());
	_SSD_c.vmax			= _avoidance_config.getVMax();
	_SSD_c.vset			= _avoidance_config.getVSet();
	const double vset_out		= _SSD_c.vset + 0.1;

	double angle_step	= 2. * M_PI / N_angle;
	double angle		= 0.;
	double cos_angle;
	double sin_angle;
	std::vector<double> x_c;	// x coordinates of unit circle CCW
	std::vector<double> y_c;	// y coordinates of unit circle CCW
	for (unsigned i = 0; i < N_angle; ++i)
	{
		cos_angle = cos(angle);
		sin_angle = sin(angle);
		x_c.push_back(cos_angle);
		y_c.push_back(sin_angle);
		angle = angle + angle_step;
	}

	unsigned j; // for CW polygon
	for (unsigned i = 0; i < N_angle; ++i)
	{
		j = N_angle - 1 - i;
		_SSD_c.v_c_out << ClipperLib::IntPoint(Scale_to_clipper(x_c.at(i) * _SSD_c.vmax), Scale_to_clipper(y_c.at(i) * _SSD_c.vmax)); // CCW
		_SSD_c.v_c_in << ClipperLib::IntPoint(Scale_to_clipper(x_c.at(j) * _SSD_c.vmin), Scale_to_clipper(y_c.at(j) * _SSD_c.vmin)); // CW
		_SSD_c.v_c_set_out << ClipperLib::IntPoint(Scale_to_clipper(x_c.at(i) * vset_out), Scale_to_clipper(y_c.at(i) * vset_out)); // CCW
		_SSD_c.v_c_set_in << ClipperLib::IntPoint(Scale_to_clipper(x_c.at(j) * _SSD_c.vset), Scale_to_clipper(y_c.at(j) * _SSD_c.vset)); // CW
	}
	return 0;
}

int AvoidanceNode::InitialiseLogger()
{
	_logging = _avoidance_config.getLogging();
	if (_logging)
	{
		std::string prefix = _avoidance_config.getLogPrefix();
		time_t t = time(NULL);
		struct tm *tm = gmtime(&t);
		std::string filename =  std::to_string(tm->tm_year + 1900) + "-" +
					std::to_string(tm->tm_mon + 1) + "-" +
					std::to_string(tm->tm_mday) + "-" +
					std::to_string(tm->tm_hour) + "-" +
					std::to_string(tm->tm_min) + "-" +
					std::to_string(tm->tm_sec) + "_" + prefix;
		_logfile.open("avoidance_logs/" + filename + ".log");
		_logfile << std::setprecision(20);
		_logfile << "# Configuration: t_lookahead = " << _avoidance_config.getTLookahead() <<
			    ", t_pop_traffic = " << _avoidance_config.getTPopTraffic() <<
			    ", r_px = " << _avoidance_config.getRPZ() <<
			    ", r_pz_mar = " << _avoidance_config.getRPZMar() <<
			    ", n_angle = " << _avoidance_config.getNAngle() <<
			    ", v_min = " << _avoidance_config.getVMin() <<
			    ", v_max = " << _avoidance_config.getVMax() <<
			    ", v_set = " << _avoidance_config.getVSet() << "\n";
		_logfile << "timelog, timegps, lat, lon, alt, vn, ve, vd, gps_valid, avoid_vn, avoid_ve, avoid_vd, avoid\n";

		_traffic_logfile.open("avoidance_logs/" + filename + "_traffic.log");
		_traffic_logfile << std::setprecision(20);
		_traffic_logfile << "# Configuration: t_lookahead = " << _avoidance_config.getTLookahead() <<
			    ", t_pop_traffic = " << _avoidance_config.getTPopTraffic() <<
			    ", r_px = " << _avoidance_config.getRPZ() <<
			    ", r_pz_mar = " << _avoidance_config.getRPZMar() <<
			    ", n_angle = " << _avoidance_config.getNAngle() <<
			    ", v_min = " << _avoidance_config.getVMin() <<
			    ", v_max = " << _avoidance_config.getVMax() <<
			    ", v_set = " << _avoidance_config.getVSet() << "\n";
		_traffic_logfile << "timelog, acid, lat, lon, alt, hdg, groundspeed, recorded_time, delay\n";
	}
	return 0;
}

int AvoidanceNode::handle_periodic_timer()
{
	// every 200ms send velocity vector if a packet was missing
	AvoidanceVelocity avoidance_velocity;
	avoidance_velocity.set_avoid(_avoid);
	avoidance_velocity.set_vn(_vn_sp * 1000);
	avoidance_velocity.set_ve(_ve_sp * 1000);
	avoidance_velocity.set_vd(_vd_sp * 1000);
	avoidance_velocity.set_lat(static_cast<int>(_latd_sp * 1e7));
	avoidance_velocity.set_lon(static_cast<int>(_lond_sp * 1e7));
	std::string request = avoidance_velocity.SerializeAsString();
	//_avoidance_req.send_request(request);
	_avoidance_pub.publish(request);

	// perform traffic housekeeping
	traffic_housekeeping(_avoidance_config.getTPopTraffic());
	// update time to target and lookaheadtime
	_t_lookahead = _avoidance_config.getTLookahead();
	if (_target_wp_available)
	{
		latdlond target_latdlond = _avoidance_geometry.getWpCoordinatesLatLon(_target_wp);
		double distance_to_target = calc_distance_from_reference_and_target_latdlond(_own_pos, target_latdlond);
		double speed = sqrt(pow(_ve,2) + pow(_vn,2));
		double time_to_target = distance_to_target / speed;
		_t_lookahead = std::min(time_to_target, _t_lookahead);
	}
	for (Avoidance_intruder intruder : _intruders)
	{
		statebased_CD(intruder);
	}
	if (_intr_inconf.size() > 0)
	{
		_in_pz = false;
		ConstructSSD();
		SSDResolution();
	}
	if (_avoid)
	{
		ResumeNav();
	}

	// logging
	if (_logging)
	{
		write_log();
	}

	return 0;
}

void AvoidanceNode::traffic_callback(std::string message)
{
	TrafficInfo traffic_info;
	bool success = traffic_info.ParseFromString(message);
	if (success) {
		handle_traffic(traffic_info);
	}
}

int AvoidanceNode::handle_traffic(const TrafficInfo &traffic)
{
	std::string aircraft_id_i = traffic.aircraft_id();
	if (aircraft_id_i.find("flight|") == std::string::npos)
	{
		return 0;
	}
	double latd_i = traffic.lat() * 1e-7;
	double lond_i = traffic.lon() * 1e-7;
	double alt_i = traffic.alt() * 1e-3;
	double hdg_i = traffic.heading();
	double gs_i = traffic.groundspeed() * AVOIDANCE_KTS * 1e-3;
	double recorded_time_i = traffic.recorded_time();// * 1e-3;
	double delay = getTimeStamp() - recorded_time_i;
	if (_logging)
	{
		write_traffic_log(aircraft_id_i, latd_i, lond_i, alt_i, hdg_i, gs_i, recorded_time_i, delay);
	}
//	std::cout << "Delay: " << delay << std::endl;

	// Check if traffic already exists in traffic vector
	bool intruder_match = false;
	for (Avoidance_intruder& intruder : _intruders)
	{
		if (intruder.getAircraftId() == aircraft_id_i)
		{
			intruder_match = true;
			intruder.setData(latd_i, lond_i, alt_i, hdg_i, gs_i, recorded_time_i);
			intruder.updateRelVar(_own_pos.lat, _own_pos.lon, _own_pos.alt, _vn, _ve, _own_pos.r);
			//std::cout << "updating id: " << intruder.getAircraftId() << std::endl;
			break;
		}
	}
	if (!intruder_match)
	{
		Avoidance_intruder intruder(aircraft_id_i, latd_i, lond_i, alt_i, hdg_i, gs_i, recorded_time_i);
		intruder.updateRelVar(_own_pos.lat, _own_pos.lon, _own_pos.alt, _vn, _ve, _own_pos.r);
		_intruders.push_back(intruder);
		std::cout << "adding id: " << aircraft_id_i << std::endl;
	}

	if (!_gps_position_valid) {
		std::cerr << "Unknown position of self" << std::endl;
		return -1;
	}
}

void AvoidanceNode::gps_position_callback(std::string message)
{
	GPSMessage gps_info;
	bool success = gps_info.ParseFromString(message);
	if (success) {
		handle_gps_position(gps_info);
	}
}

int AvoidanceNode::handle_gps_position(const GPSMessage &gps_info)
{
	_lat = gps_info.lat() * 1e-7;
	_lon = gps_info.lon() * 1e-7;
	_alt = gps_info.alt_msl() * 1e-3;
	_vn = gps_info.vn() * 1e-3;
	_ve = gps_info.ve() * 1e-3;
	_vd = gps_info.vd() * 1e-3;
	_target_wp_available = gps_info.has_target_wp();
	if (_target_wp_available)
	{
		_target_wp = gps_info.target_wp();
	}
	_gps_position_valid = true;
	_time_gps = getTimeStamp();

	update_position_params(_own_pos, _lat, _lon, _alt);
	//std::cout << "Got GPS: " << std::to_string(gps_info.lat() * 1e-7) << ", " << std::to_string(gps_info.lon() * 1e-7) << std::endl;
}

int AvoidanceNode::get_relative_coordinates(double lat_0, double lon_0, double lat_i, double lon_i, double *x, double *y)
{

	const double lat_0_rad = lat_0 * M_PI / 180.0;
	const double lon_0_rad = lon_0 * M_PI / 180.0;
	const double sin_lat_0 = std::sin(lat_0_rad);
	const double cos_lat_0 = std::cos(lat_0_rad);

	const double lat_i_rad = lat_i * M_PI / 180.0;
	const double lon_i_rad = lon_i * M_PI / 180.0;
	const double sin_lat_i = std::sin(lat_i_rad);
	const double cos_lat_i = std::cos(lat_i_rad);

	const double cos_d_lon = std::cos(lon_i_rad - lon_0_rad);

	double arg = sin_lat_0 * sin_lat_i + cos_lat_0 * cos_lat_i * cos_d_lon;
	if (arg > 1.0) {
		arg = 1.0;
	}
	if (arg < -1.0) {
		arg = -1.0;
	}
	const double c = std::acos(arg);

	double k = 1.0;

	if (fabs(c) > 0) {
		k = (c / std::sin(c));
	}

	*x = k * (cos_lat_0 * sin_lat_i - sin_lat_0 * cos_lat_i * cos_d_lon) * 6371000;
	*y = k * cos_lat_i * std::sin(lon_i_rad - lon_0_rad) * 6371000;

	return 0;
}

double AvoidanceNode::getTimeStamp()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return tp.tv_sec + tp.tv_usec * 1e-6;
}

//void AvoidanceNode::avoidance_reply_callback(EventSource *es)
//{
//	Requester* req = static_cast<Requester*>(es);
//	AvoidanceNode* node = static_cast<AvoidanceNode*>(es->_target_object);

//	std::string reply = req->get_reply();
//	if (reply != "OK") {
//		std::cout << reply << std::endl;
//	}
//}

void AvoidanceNode::write_log()
{
	_logfile << getTimeStamp() << ", " <<
		    _time_gps << ", " <<
		    _lat << ", " <<
		    _lon << ", " <<
		    _alt << ", " <<
		    _vn << ", " <<
		    _ve << ", " <<
		    _vd << ", " <<
		    _gps_position_valid << ", " <<
		    _vn_sp << ", " <<
		    _ve_sp << ", " <<
		    _vd_sp << ", " <<
		    _avoid << "\n";
}

void AvoidanceNode::write_traffic_log(std::string acid, double latd, double lond, double alt, double hdg, double gs, double recorded_time, double delay)
{
	_traffic_logfile << getTimeStamp() << ", " <<
			    acid << ", " <<
			    latd << ", " <<
			    lond << ", " <<
			    alt << ", " <<
			    hdg << ", " <<
			    gs << ", " <<
			    recorded_time << ", " <<
			    delay << "\n";
}

void AvoidanceNode::traffic_housekeeping(double t_pop_traffic)
{
	// cleaning up the traffic array
	double time = getTimeStamp();
	unsigned i = 0;
	bool looping = _intruders.size();

	while (looping)
	{
		Avoidance_intruder intruder = _intruders.at(i);
		std::string ac_id = intruder.getAircraftId();
		if ((time - intruder.getReceivedTime()) > t_pop_traffic)
		{
			std::cout << "popping id: " << ac_id << std::endl;
			if (intruder.getInConf())
			{
				_intr_inconf.erase(ac_id);
			}
			if (intruder.getAvoiding())
			{
				_intr_avoid.erase(ac_id);
			}
			_intruders.erase(_intruders.begin() + i);
			if (i >= _intruders.size())
			{
				looping = false;
			}
		}
		else
		{
			if ((i + 1) < _intruders.size())
			{
				i++;
			}
			else
			{
				looping = false;
			}
		}
	}
}

void AvoidanceNode::statebased_CD(Avoidance_intruder& intruder)
{
	double rpz;
	double rpz2;

	rpz = _avoidance_config.getRPZ() * _avoidance_config.getRPZMarDetect();
	rpz2 = pow(rpz,2);

	const std::string ac_id = intruder.getAircraftId();

	// Relative variables
	double pn_rel = intruder.getPnRel();
	double pe_rel = intruder.getPeRel();
	double vn_rel = intruder.getVnRel();
	double ve_rel = intruder.getVeRel();
	double v2_rel = intruder.getV2Rel();
	double v_rel  = intruder.getVRel();
	double d2_rel = intruder.getD2Rel();

	// calculation of horizontal conflict variables
	double t_cpa = - (pn_rel * vn_rel + pe_rel * ve_rel) / (v2_rel);
	double d_cpa = sqrt(d2_rel - pow(t_cpa, 2) * v2_rel);
	double d2_cpa = pow(d_cpa, 2);

	double d_in = 0;
	double d_los = 1e6;
	double t_los = 1e6;
	double t_out = 1e6;
	if ((d_cpa < rpz) && (t_cpa > 0))
	{
		d_in = sqrt(rpz2 - d2_cpa);
		d_los = v_rel * t_cpa - d_in;
		double dt_in = d_in / v_rel;
		t_los = t_cpa - dt_in;
		t_out = t_cpa + dt_in;
	}
	bool inconf = (d_cpa < rpz) && (t_los < _t_lookahead) && (t_out > 0);
	intruder.setConflictPar(inconf, t_cpa, d_cpa, d_in, d_los, t_los);

	if (inconf)
	{
		// add/overwrite intruder to avoid and inconf map
		_intr_inconf[ac_id] = &intruder;
		_intr_avoid[ac_id] = &intruder;
		//std::cout << "In Conflict with: " << ac_id << " in " << t_los << " seconds" << " \t d_cpa: " << d_cpa << std::endl;
	}
	else {
		if ( _intr_inconf.find(ac_id) == _intr_inconf.end() ) {
			// not found
		} else {
			// found
			_intr_inconf.erase(ac_id);
		}
	}
}

int AvoidanceNode::ConstructSSD()
{
	size_t ntraf = _intruders.size();
	_SSD_v.ARV_scaled.clear();
	ClipperLib::Clipper c;
	c.AddPath(_SSD_c.v_c_out, ClipperLib::ptSubject, true);
	c.AddPath(_SSD_c.v_c_in, ClipperLib::ptSubject, true);

	// Initialise vector of velocity obstacles for intruders
	std::vector<ClipperLib::Path> VO_i_paths;

	if (ntraf > 0)
	{
		for (Avoidance_intruder intruder: _intruders)
		{
			double vn_i = intruder.getVn();
			double ve_i = intruder.getVe();
			double qdr = intruder.getBearingRel();
			double dist = intruder.getDRel();
			double hsep = _avoidance_config.getRPZ();
			double hsepm = _SSD_c.hsepm;

			// In LOS the VO can't be defined, place the distance on the edge
			if (dist < hsep)
			{
				_in_pz = true;
			}
			if (dist < hsepm)
			{
				dist = hsepm;
			}

			double alpha = asin(hsepm / dist);
			if (alpha > _SSD_c.alpham)
			{
				alpha = _SSD_c.alpham;
			}

			// Relevant sin/cos/tan
			double sinqdr = sin(qdr); // East
			double cosqdr = cos(qdr); // North
			double tanalpha = tan(alpha);
			double cosqdrtanalpha = cosqdr * tanalpha;
			double sinqdrtanalpha = sinqdr * tanalpha;

			double e1 = (sinqdr + cosqdrtanalpha) * 2. * _SSD_c.vmax;
			double e2 = (sinqdr - cosqdrtanalpha) * 2. * _SSD_c.vmax;
			double n1 = (cosqdr - sinqdrtanalpha) * 2. * _SSD_c.vmax;
			double n2 = (cosqdr + sinqdrtanalpha) * 2. * _SSD_c.vmax;

			//std::cout << "e1: " << e1 << " e2: " << e2 << " n1: " << n1 << " n2: " << n2 << std::endl;

			ClipperLib::Path VO_int;
			VO_int << ClipperLib::IntPoint(Scale_to_clipper(ve_i), Scale_to_clipper(vn_i))
			       << ClipperLib::IntPoint(Scale_to_clipper(e1 + ve_i), Scale_to_clipper(n1 + vn_i))
			       << ClipperLib::IntPoint(Scale_to_clipper(e2 + ve_i), Scale_to_clipper(n2 + vn_i));
			VO_i_paths.push_back(VO_int);
			c.AddPath(VO_i_paths.at(VO_i_paths.size() - 1), ClipperLib::ptClip, true);
		}
	}
	_SSD_v.ARV_scaled.clear();
	c.Execute(ClipperLib::ctDifference, _SSD_v.ARV_scaled, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
	return 0;
}

int AvoidanceNode::SSDResolution()
{
	// Assuming constant speed (for now) !!!!!!!

	ClipperLib::Clipper c;
	c.AddPaths(_SSD_v.ARV_scaled, ClipperLib::ptSubject, true);
	c.AddPath(_SSD_c.v_c_set_out, ClipperLib::ptClip, true);
	c.AddPath(_SSD_c.v_c_set_in, ClipperLib::ptClip, true);
	_SSD_v.ARV_scaled_speed.clear();
	c.Execute(ClipperLib::ctIntersection, _SSD_v.ARV_scaled_speed, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

	// Clip geofences
	for (std::pair<std::string, Avoidance_intruder*> intruder_pair : _intr_inconf)
	{
		Avoidance_intruder* intruder = intruder_pair.second;
		c.AddPaths(ConstructGeofencePolygons(*intruder), ClipperLib::ptClip, true);
	}
	_SSD_v.ARV_scaled_geofence.clear();
	c.Execute(ClipperLib::ctDifference, _SSD_v.ARV_scaled_geofence, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

	struct Vertex_data {
		double p_e;
		double p_n;
		double q_e;
		double q_n;
		double l;
		double t;
		double w_e;
		double w_n;
		double d2;
	};
	std::vector<Vertex_data> vertices;
	double vn;
	double ve;
	if (_avoidance_config.getAvoidToTarget() && _target_wp_available) // Avoid towards next waypoint as target heading
	{
		n_e_coordinate relative_wp = _avoidance_geometry.getRelWpNorthEast(_own_pos, _target_wp);
		double rel_hdg = atan2(relative_wp.east, relative_wp.north);
		vn = _SSD_c.vset * cos(rel_hdg);
		ve = _SSD_c.vset * sin(rel_hdg);
	}
	else // Avoid with current heading as target
	{
		vn = _vn;
		ve = _ve;
	}


	// if no solution
	if (_SSD_v.ARV_scaled.size() == 0)
	{
		std::cerr << "No Solution for conflict found!!!" << std::endl;
		return -1;
	}

	for (ClipperLib::Path path : _SSD_v.ARV_scaled_speed)
	{

		for (unsigned i = 0; i < path.size(); ++i)
		{
			ClipperLib::IntPoint point = path.at(i);
			ClipperLib::IntPoint point_next;
			if (i < (path.size() - 1))
			{
				point_next = path.at(i + 1);
			}
			else {
				point_next = path.at(0);
			}
			Vertex_data vertex;
			double Ve = Scale_from_clipper(point.X);
			double Vn = Scale_from_clipper(point.Y);
			double Ve_next = Scale_from_clipper(point_next.X);
			double Vn_next = Scale_from_clipper(point_next.Y);
			double Ve_diff = Ve_next - Ve;
			double Vn_diff = Vn_next - Vn;
			double diff2 = pow(Ve_diff, 2.) + pow(Vn_diff, 2.);
			double t_scalar = ((ve - Ve) * Ve_diff + (vn - Vn) * Vn_diff) / diff2;
			t_scalar = std::max(0., std::min(t_scalar, 1.)); // clip between 0. and 1.
			double Ve_res = Ve + t_scalar * Ve_diff;
			double Vn_res = Vn + t_scalar * Vn_diff;
			vertex.p_e = Ve;
			vertex.p_n = Vn;
			vertex.q_e = Ve_diff;
			vertex.q_n = Vn_diff;
			vertex.l = diff2;
			vertex.t = t_scalar;
			vertex.w_e = Ve_res;
			vertex.w_n = Vn_res;
			vertex.d2 = pow(Ve_res - ve, 2.) + pow(Vn_res - vn, 2.);
			vertices.push_back(vertex);
		}
		// sort solutions
		// sort by d2:
		std::sort(vertices.begin(), vertices.end(),
		    [](Vertex_data const &a, Vertex_data const &b) {
			return a.d2 < b.d2;
		    });

		_ve_sp = vertices[0].w_e;
		_vn_sp = vertices[0].w_n;
		_avoid = true;

		// Calculate CPA for solution
		double max_t_cpa_res = 0;
		for (std::pair<std::string, Avoidance_intruder*> intruder_pair : _intr_inconf)
		{
			Avoidance_intruder* intruder = intruder_pair.second;
			double vn_res_rel = _vn_sp - intruder->getVn();
			double ve_res_rel = _ve_sp - intruder->getVe();
			double v_res_rel2 = pow(vn_res_rel,2) + pow(ve_res_rel,2);
			double t_cpa_res = - (intruder->getPnRel() * vn_res_rel + intruder->getPeRel() * ve_res_rel) / (v_res_rel2);
			max_t_cpa_res = std::max(max_t_cpa_res, t_cpa_res);
		}
		n_e_coordinate cpa_ne;
		cpa_ne.north = _vn_sp * max_t_cpa_res; // [m]
		cpa_ne.east  = _ve_sp * max_t_cpa_res; // [m]

		latdlond res_point = calc_latdlond_from_reference(_own_pos, cpa_ne);
		std::vector<std::vector<double>> soft_geofence = _avoidance_geometry.getSoftGeofenceLatdLond();
		bool res_point_in_geofence = latdlond_inside_geofence(_own_pos, soft_geofence, res_point);

		if (res_point_in_geofence)
		{
			_latd_sp = res_point.latd;
			_lond_sp = res_point.lond;
		}

		// Generate avoidance message
		AvoidanceVelocity Avoidance_msg;
		Avoidance_msg.set_vd(0);
		Avoidance_msg.set_ve(static_cast<int>(_ve_sp * 1000.));
		Avoidance_msg.set_vn(static_cast<int>(_vn_sp * 1000.));
		Avoidance_msg.set_avoid(_avoid);
		Avoidance_msg.set_lat(static_cast<int>(_latd_sp * 1e7));
		Avoidance_msg.set_lon(static_cast<int>(_lond_sp * 1e7));
		std::string avoidance_message = Avoidance_msg.SerializeAsString();
		_avoidance_pub.publish(avoidance_message);
	}
	return 0;
}

ClipperLib::Paths AvoidanceNode::ConstructGeofencePolygons(Avoidance_intruder &intruder)
{
	ClipperLib::Paths geofence_polygons;
	double ref_r		= _own_pos.r;
	double ref_coslat	= _own_pos.coslat;
	double p_int_n		= intruder.getPnRel();
	double p_int_e		= intruder.getPeRel();
	double v_int_n		= intruder.getVn();
	double v_int_e		= intruder.getVe();

	std::vector<std::vector<double>> geofence_ll = _avoidance_geometry.getSoftGeofenceLatdLond();
	std::vector<n_e_coordinate> geofence_ne;

	for (size_t i = 0; i < geofence_ll.size(); i++)
	{
		double lat_diff = geofence_ll.at(i)[1] - _own_pos.lat;
		double lon_diff = geofence_ll.at(i)[0] - _own_pos.lon;

		n_e_coordinate geo_ne;
		geo_ne.north	= lat_diff * ref_r;
		geo_ne.east	= lon_diff * ref_r * ref_coslat;

		geofence_ne.push_back(geo_ne);
	}

	std::vector<geofence_sector> geofence_sections;

	for (size_t i = 0; i < geofence_ne.size(); i++)
	{
		size_t i_next;
		if (i == (geofence_ne.size() - 1))
		{
			i_next = 0;
		}
		else
		{
			i_next = i + 1;
		}
		std::array<n_e_coordinate,2> coordinates_ne;
		coordinates_ne[0] = geofence_ne.at(i);
		coordinates_ne[1] = geofence_ne.at(i_next);
		geofence_sector geo_sec = calc_geofence_sector_from_n_e(coordinates_ne);
		geofence_sections.push_back(geo_sec);
	}

	for (size_t i = 0; i < geofence_sections.size(); i++)
	{
		double rotation_geo	= geofence_sections.at(i).rotation;
		double v_int_perp	= v_int_n * cos(rotation_geo) - v_int_e * sin(rotation_geo);
		double v_int_parallel	= v_int_e * cos(rotation_geo) + v_int_n * sin(rotation_geo);
		double d_geo		= geofence_sections.at(i).dist;
		double p_int_parallel	= p_int_e * cos(rotation_geo) + p_int_n * sin(rotation_geo);
		double p_int_perp	= p_int_n * cos(rotation_geo) - p_int_e * sin(rotation_geo);
		double phi		= 0.5 * atan2(-p_int_parallel, p_int_perp);
		double sinphi		= sin(phi);
		double cosphi		= cos(phi);
		double sinphi2		= pow(sinphi,2);
		double cosphi2		= pow(cosphi,2);

		double K_2_parallel	= 1. - p_int_perp / d_geo * sinphi2 / (cosphi2 - sinphi2);
		double K_parallel	= -sinphi * v_int_perp * (2. + p_int_perp / d_geo);
		double c_parallel	= -K_parallel / (2. * K_2_parallel);

		double K_2_perp		= 1. + p_int_perp / d_geo * cosphi2 / (cosphi2 - sinphi2);
		double K_perp		= -cosphi * v_int_perp * (2. + p_int_perp / d_geo);
		double c_perp		= -K_perp / (2. * K_2_perp);

		double K		= pow(c_parallel,2) * K_2_parallel + pow(c_perp,2) * K_2_perp - pow(v_int_perp,2);

		double a2		= K / K_2_parallel;
		double b2		= K / K_2_perp;

		size_t n_points		= _avoidance_config.getNAngle();

		std::vector<double> v_res_n_basic;
		std::vector<double> v_res_e_basic;

		// If an ellipse:
		if (b2 > 0.)
		{
			if (v_int_perp < 0.)
			{
				continue;
			}
			double a		= sqrt(a2);
			double b		= sqrt(b2);
			double a2_over_b2	= a2 / b2;

			if (a2_over_b2 > 200.)
			{
				a2_over_b2 = 200.;
			}

			double angle		= -M_PI;
			double angle_step	= 2. * M_PI / n_points;
			std::vector<double> angles;
			for (size_t i = 0; i < n_points; ++i)
			{
				double angle_converted = angle - sin(angle * 2.) * a2_over_b2 / 400.;
				angles.push_back(angle_converted);
				v_res_n_basic.push_back(a * sin(angle));
				v_res_e_basic.push_back(b * cos(angle));
				angle = angle + angle_step;
			}

		}
		// if an hyperbola
		else
		{
			double a		= sqrt(a2);
			double b		= sqrt(-b2);
			double x		= abs(c_parallel) + 2. * _avoidance_config.getVMax();
			double angle_max	= log(x / a + sqrt(pow(x,2) / a2 - 1.));

			if (phi >= 0.)
			{
				double angle = angle_max;
				double angle_step = -2. * angle_max / n_points;
				for (size_t i = 0; i < n_points; ++i)
				{
					v_res_n_basic.push_back(b * sinh(angle));
					v_res_e_basic.push_back(a * cosh(angle));
					angle = angle + angle_step;
				}
			}
			else
			{
				double angle = -angle_max;
				double angle_step = 2. * angle_max / n_points;
				for (size_t i = 0; i < n_points; ++i)
				{
					v_res_n_basic.push_back(b * sinh(angle));
					v_res_e_basic.push_back(-a * cosh(angle));
					angle = angle + angle_step;
				}
			}
		}

		ClipperLib::Path geofence_polygon;

		double cos_rot_phi = cos(phi + rotation_geo);
		double sin_rot_phi = sin(phi + rotation_geo);

		double c_n = c_perp * cos_rot_phi + c_parallel * sin_rot_phi + v_int_parallel * sin(rotation_geo);
		double c_e = c_parallel * cos_rot_phi - c_perp * sin_rot_phi + v_int_parallel * cos(rotation_geo);

		for (size_t i = 0; i < n_points; ++i)
		{
			double v_res_n_rotated = v_res_n_basic.at(i) * cos_rot_phi + v_res_e_basic.at(i) * sin_rot_phi;
			double v_res_e_rotated = v_res_e_basic.at(i) + cos_rot_phi - v_res_n_basic.at(i) * sin_rot_phi;
			double v_res_n = v_res_n_rotated + c_n;
			double v_res_e = v_res_e_rotated + c_e;

			geofence_polygon << ClipperLib::IntPoint(Scale_to_clipper(v_res_e), Scale_to_clipper(v_res_n));
		}

		geofence_polygons.push_back(geofence_polygon);
	}
	return geofence_polygons;
}

int AvoidanceNode::ResumeNav()
{
	// resume navigation allowed if not in hor_los, and past CPA

	// loop over intruders for which an avoid maneuvre has been triggered
	std::vector<std::string> pop_keys;

	for (std::pair<std::string, Avoidance_intruder*> intruder_pair : _intr_avoid)
	{
		bool outside_hor_loss;
		bool past_cpa;
		Avoidance_intruder* intruder = intruder_pair.second;

		// check if outside hor_los (including margin)
		if (intruder->getDRel() > _SSD_c.hsepm)
		{
			outside_hor_loss = true;
		}
		else {
			outside_hor_loss = false;
		}

		// check if past cpa
		double dot_dist_vrel = intruder->getPeRel() * intruder->getVeRel() + intruder->getPnRel() * intruder->getVnRel();
		if (dot_dist_vrel > 0)
		{
			past_cpa = true;
		}
		else {
			past_cpa = false;
		}

		// combine
		if (outside_hor_loss && past_cpa)
		{
			pop_keys.push_back(intruder->getAircraftId());
		}

	}

	// remove aircraft from avoid list that are outside hlos and past cpa
	for (std::string pop_key : pop_keys)
	{
		_intr_avoid.erase(pop_key);
	}

	// unset avoidance variable
	if (_intr_avoid.size() == 0)
	{
		_avoid = false;
		AvoidanceVelocity Avoidance_msg;
		Avoidance_msg.set_vd(0);
		Avoidance_msg.set_ve(static_cast<int>(_ve_sp * 1000.));
		Avoidance_msg.set_vn(static_cast<int>(_vn_sp * 1000.));
		Avoidance_msg.set_avoid(_avoid);
		Avoidance_msg.set_lat(static_cast<int>(_latd_sp * 1e7));
		Avoidance_msg.set_lon(static_cast<int>(_lond_sp * 1e7));
		std::string avoidance_message = Avoidance_msg.SerializeAsString();
		_avoidance_pub.publish(avoidance_message);
		std::cout << "Resuming Nav" << std::endl;
	}
	return 0;
}

