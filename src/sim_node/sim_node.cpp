#include <unistd.h>
#include <cmath>

#include "vutura_common/config.hpp"
#include "vutura_common/helper.hpp"
//#include "vutura_common/event_loop.hpp"
//#include "vutura_common/timer.hpp"

#include "vutura_common.pb.h"

#include "sim_node.hpp"

SimNode::SimNode(int instance) :
	_periodic_timer(this),
	_gps_pub(socket_name(SOCK_PUBSUB_GPS_POSITION, instance)),
	_angle(0.0f)
{

}

SimNode::~SimNode()
{

}

void SimNode::init()
{
	_periodic_timer.set_timeout_callback(std::bind(&SimNode::handle_periodic_timer, this));
	_periodic_timer.start_periodic(500);
}

int SimNode::handle_periodic_timer()
{

	_angle += 1.0 *  M_PI / 180;
	if (_angle >= 360 * M_PI / 180) {
		_angle = 0.0;
	}


	float lat_0 = 52.17006269856014; // valkenburg
	float lon_0 = 4.4194865226745605;

//	float lat_0 = 48.58605; // france (podium)
//	float lon_0 =   2.32681;

	float lon = lon_0 + 0.003 * std::cos(_angle);
	float lat = lat_0 + 0.002 * std::sin(_angle);

	GPSMessage gps_message;
	uint16_t len;
	gps_message.set_timestamp(0);
	gps_message.set_lat(lat*1e7);
	gps_message.set_lon(lon*1e7);
	gps_message.set_alt_msl(10000);
	gps_message.set_alt_agl(10000);

	std::string message_string = gps_message.SerializeAsString();

	_gps_pub.publish(message_string);

	std::cout << "lat: " << lat << "\tlon: " << lon << std::endl;

	return 0;
}
