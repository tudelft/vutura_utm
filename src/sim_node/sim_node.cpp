#include <unistd.h>
#include <cmath>

#include "vutura_common/event_loop.hpp"
#include "vutura_common/timer.hpp"

#include "vutura_common.pb.cc"

#include "sim_node.hpp"

SimNode::SimNode(int instance) :
	gps_pub(socket_name(SOCK_PUBSUB_GPS_POSITION, instance)),
	_angle(0.0f)
{

}

int SimNode::handle_periodic_timer()
{

	_angle += 1.0 *  M_PI / 180;
	if (_angle >= 360 * M_PI / 180) {
		_angle = 0.0;
	}

	float lon = 4.4194865226745605 + 0.002 * std::cos(_angle);
	float lat = 52.17006269856014 + 0.002 * std::sin(_angle);

	GPSMessage gps_message;
	uint16_t len;
	gps_message.set_timestamp(0);
	gps_message.set_lat(lat*1e7);
	gps_message.set_lon(lon*1e7);
	gps_message.set_alt_msl(50000);
	gps_message.set_alt_agl(50000);

	gps_pub.publish(gps_message.SerializeAsString());

	std::cout << "lat: " << lat << "\tlon: " << lon << std::endl;

	return 0;
}

void SimNode::heartbeat_timer_callback(EventSource *es)
{
	Timer* tim = static_cast<Timer*>(es);
	SimNode* node = static_cast<SimNode*>(es->_target_object);
	uint64_t num_timer_events;
	ssize_t recv_size = read(es->_fd, &num_timer_events, 8);
	(void) recv_size;

	node->handle_periodic_timer();
}
