#include <unistd.h>
#include <cmath>
#include <algorithm>
#include "sys/time.h"

#include "vutura_common/event_loop.hpp"
#include "vutura_common/timer.hpp"
#include "vutura_common/subscription.hpp"

#include "avoidance_node.hpp"

AvoidanceNode::AvoidanceNode(int instance, Avoidance_config& config, Avoidance_geometry& geometry) :
	_avoidance_req(this, socket_name(SOCK_REQREP_AVOIDANCE_COMMAND, instance), avoidance_reply_callback),
	_avoidance_config(config),
	_avoidance_geometry(geometry),
	_gps_position_valid(false),
	_lat(0),
	_lon(0),
	_alt(0),
        _vn(0),
        _ve(0),
        _vd(0),
	_avoid(false),
        _vn_sp(0),
        _ve_sp(0),
        _vd_sp(0),
        _intruders()
{

}

int AvoidanceNode::InitialiseSSD()
{
        // Import configuration and copy relevant settings for SSD
        const unsigned N_angle          = _avoidance_config.getNAngle();
        const double hsep               = _avoidance_config.getRPZ();
        const double margin             = _avoidance_config.getRPZMar();
        _SSD_c.hsepm                    = hsep * margin;
        _SSD_c.alpham                   = 0.4999 * M_PI;
        _SSD_c.vmin                     = std::max(0.1, _avoidance_config.getVMin());
        _SSD_c.vmax                     = _avoidance_config.getVMax();
        _SSD_c.vset                     = _avoidance_config.getVSet();
        const double vset_out           = _SSD_c.vset + 0.1;

        double angle_step = 2. * M_PI / N_angle;
        double angle = 0.;
        double cos_angle;
        double sin_angle;
        std::vector<double> x_c;        // x coordinates of unit circle CCW
        std::vector<double> y_c;        // y coordinates of unit circle CCW
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

int AvoidanceNode::handle_periodic_timer()
{
	// every 200ms send velocity vector
	AvoidanceVelocity avoidance_velocity;
	avoidance_velocity.set_avoid(_avoid);
        avoidance_velocity.set_vn(_vn_sp * 1000);
        avoidance_velocity.set_ve(_ve_sp * 1000);
        avoidance_velocity.set_vd(_vd_sp * 1000);
	std::string request = avoidance_velocity.SerializeAsString();
	_avoidance_req.send_request(request);

        // perform traffic housekeeping
        traffic_housekeeping(_avoidance_config.getTPopTraffic());
        for (Avoidance_intruder intruder : _intruders)
        {
                statebased_CD(intruder);
        }
        if (_intr_inconf.size() > 0)
        {
                ConstructSSD();
                SSDResolution();
        }
        if (_avoid)
        {
                std::cout << "ResumeNav function" << std::endl;
                ResumeNav();
        }


	return 0;
}

int AvoidanceNode::handle_traffic(const TrafficInfo &traffic)
{       
        std::string aircraft_id_i = traffic.aircraft_id();
        if (aircraft_id_i.find("flight|"))
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
//        std::cout << "Delay: " << delay << std::endl;

//	if(alt_i > 120) {
//		return 0;
//	}

        // Check if traffic already exists in traffic vector
        bool intruder_match = false;
        for (Avoidance_intruder& intruder : _intruders)
        {
                if (intruder.getAircraftId() == aircraft_id_i)
                {
                        intruder_match = true;
                        intruder.setData(latd_i, lond_i, alt_i, hdg_i, gs_i, recorded_time_i);
                        intruder.updateRelVar(_own_pos.lat, _own_pos.lon, _own_pos.alt, _vn, _ve, _own_pos.r);
                        std::cout << "updating id: " << intruder.getAircraftId() << std::endl;
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

	// check distance to self
//	double x,y,dist;
//      get_relative_coordinates(_lat, _lon, latd_i, lond_i, &x, &y);
//	dist = sqrt(x * x + y * y);
//	std::cout << "Traffic: " << std::to_string(x) << ", " << std::to_string(y) << "\tdist: " << dist << "\talt: " << alt_i << std::endl;

        // Worst avoidance algorithm in the world:
//	if (alt_i < 120 && dist < 2000) {
//		_vx_sp = x/dist * 6;
//		_vy_sp = y/dist * 6;
//		_vz_sp = 0;

//		_avoid = true;
//	} else {
//		//_avoid = false;
//	}
}

int AvoidanceNode::handle_gps_position(const GPSMessage &gps_info)
{
	_lat = gps_info.lat() * 1e-7;
	_lon = gps_info.lon() * 1e-7;
	_alt = gps_info.alt_msl() * 1e-3;
        _vn = gps_info.vn() * 1e-3;
        _ve = gps_info.ve() * 1e-3;
        _vd = gps_info.vd() * 1e-3;
	_gps_position_valid = true;

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

void AvoidanceNode::periodic_timer_callback(EventSource *es)
{
	Timer* tim = static_cast<Timer*>(es);
	AvoidanceNode* node = static_cast<AvoidanceNode*>(es->_target_object);
	uint64_t num_timer_events;
	ssize_t recv_size = read(es->_fd, &num_timer_events, 8);
	(void) recv_size;

	node->handle_periodic_timer();
}

void AvoidanceNode::traffic_callback(EventSource *es)
{
	Subscription* traffic_sub = static_cast<Subscription*>(es);
	AvoidanceNode* node = static_cast<AvoidanceNode*>(es->_target_object);

	std::string message = traffic_sub->get_message();
	TrafficInfo traffic_info;
	bool success = traffic_info.ParseFromString(message);
	if (success) {
		node->handle_traffic(traffic_info);
	}
}

void AvoidanceNode::gps_position_callback(EventSource *es)
{
	Subscription* gps_sub = static_cast<Subscription*>(es);
	AvoidanceNode* node = static_cast<AvoidanceNode*>(es->_target_object);

	std::string message = gps_sub->get_message();
	GPSMessage gps_info;
	bool success = gps_info.ParseFromString(message);
	if (success) {
		node->handle_gps_position(gps_info);
	}
}

void AvoidanceNode::avoidance_reply_callback(EventSource *es)
{
	Requester* req = static_cast<Requester*>(es);
	AvoidanceNode* node = static_cast<AvoidanceNode*>(es->_target_object);

	std::string reply = req->get_reply();
	if (reply != "OK") {
		std::cout << reply << std::endl;
	}
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
        const double rpz = _avoidance_config.getRPZ();
        const double rpz2 = pow(rpz, 2);
        const double t_lookahead = _avoidance_config.getTLookahead();
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
        bool inconf = (d_cpa < rpz) && (t_los < t_lookahead) && (t_out > 0);
        intruder.setConflictPar(inconf, t_cpa, d_cpa, d_in, d_los, t_los);

        if (inconf)
        {
                // add/overwrite intruder to avoid and inconf map
                _intr_inconf[ac_id] = &intruder;
                _intr_avoid[ac_id] = &intruder;
                std::cout << "In Conflict with: " << ac_id << " in " << t_los << " seconds" << " \t d_cpa: " << d_cpa << std::endl;
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

signed long long AvoidanceNode::Scale_to_clipper(double coord)
{
        signed long long clipper_coord = static_cast<signed long long>(coord * pow(2., 31));
        return clipper_coord;
}

double AvoidanceNode::Scale_from_clipper(double coord)
{
        double double_coord = static_cast<double>(coord * pow(2., -31));
        return double_coord;
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

                        // In LOS the VO can't be defined, place the distance on the edge
                        if (dist < _SSD_c.hsepm)
                        {
                                dist = _SSD_c.hsepm;
                        }

                        double alpha = asin(_SSD_c.hsepm / dist);
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

                        ClipperLib::Path VO_int;
                        VO_int << ClipperLib::IntPoint(Scale_to_clipper(ve_i), Scale_to_clipper(vn_i))
                               << ClipperLib::IntPoint(Scale_to_clipper(e1 + ve_i), Scale_to_clipper(n1 + ve_i))
                               << ClipperLib::IntPoint(Scale_to_clipper(e2 + ve_i), Scale_to_clipper(n2 + vn_i));
                        VO_i_paths.push_back(VO_int);
                        c.AddPath(VO_i_paths.at(VO_i_paths.size() - 1), ClipperLib::ptClip, true);
                }
        }
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
        c.Execute(ClipperLib::ctIntersection, _SSD_v.ARV_scaled, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

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
        double ve_scaled = Scale_to_clipper(_ve);
        double vn_scaled = Scale_to_clipper(_vn);

        // if no solution
        if (_SSD_v.ARV_scaled.size() == 0)
        {
                std::cerr << "No Solution for conflict found!!!" << std::endl;
                return -1;
        }

        for (ClipperLib::Path path : _SSD_v.ARV_scaled)
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
                        double Ve = point.X;
                        double Vn = point.Y;
                        double Ve_next = point_next.X;
                        double Vn_next = point_next.Y;
                        double Ve_diff = Ve_next - Ve;
                        double Vn_diff = Vn_next - Vn;
                        double diff2 = pow(Ve_diff, 2.) + pow(Vn_diff, 2.);
                        double t_scalar = ((ve_scaled - Ve) * Ve_diff + (vn_scaled - Vn) * Vn_diff) / diff2;
                        double Ve_res = Ve + t_scalar * Ve_diff;
                        double Vn_res = Vn + t_scalar + Vn_diff;
                        t_scalar = std::max(0., std::min(t_scalar, 1.)); // clip between 0. and 1.
                        vertex.p_e = Ve;
                        vertex.p_n = Vn;
                        vertex.q_e = Ve_diff;
                        vertex.q_n = Vn_diff;
                        vertex.l = diff2;
                        vertex.t = t_scalar;
                        vertex.w_e = Ve_res;
                        vertex.w_n = Vn_res;
                        vertex.d2 = pow(Ve_res - ve_scaled, 2.) + pow(Vn_res - vn_scaled, 2.);
                        vertices.push_back(vertex);
                }
                // sort solutions
                // sort by name:
                std::sort(vertices.begin(), vertices.end(),
                    [](Vertex_data const &a, Vertex_data const &b) {
                        return a.d2 < b.d2;
                    });

                _ve_sp = Scale_from_clipper(vertices[0].w_e);
                _vn_sp = Scale_from_clipper(vertices[1].w_n);
                _avoid = true;
        }
        return 0;
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
                        past_cpa = true;
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
                std::cout << "Resuming Nav" << std::endl;
        }

        return 0;
}

