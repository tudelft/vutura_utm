#include <unistd.h>
#include <cmath>
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

	return 0;
}

int AvoidanceNode::handle_traffic(const TrafficInfo &traffic)
{
        std::string aircraft_id_i = traffic.aircraft_id();
        double latd_i = traffic.lat() * 1e-7;
        double lond_i = traffic.lon() * 1e-7;
        double alt_i = traffic.alt() * 1e-3;
        double hdg_i = traffic.heading();
        double gs_i = traffic.groundspeed() * 1e-3;
        double recorded_time_i = traffic.recorded_time() * 1e-3;

        std::cout << "HDG: " << hdg_i << std::endl;

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
                        intruder.updateRelPos(_own_pos.lat, _own_pos.lon, _own_pos.alt, _own_pos.r);
                        std::cout << "updating id: " << intruder.getAircraftId() << std::endl;
                        break;
                }
        }
        if (!intruder_match)
        {
                Avoidance_intruder intruder(aircraft_id_i, latd_i, lond_i, alt_i, hdg_i, gs_i, recorded_time_i);
                intruder.updateRelPos(_own_pos.lat, _own_pos.lon, _own_pos.alt, _own_pos.r);
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
                if ((time - _intruders.at(i).getReceivedTime()) > t_pop_traffic)
                {
                        std::cout << "popping id: " << _intruders.at(i).getAircraftId() << std::endl;
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

void AvoidanceNode::statebased_CD()
{
        const double rpz = _avoidance_config.getRPZ();
        const double t_lookahead = _avoidance_config.getTLookahead();
}


