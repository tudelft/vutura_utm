#pragma once
#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "vutura_common/udp_source.hpp"
#include "vutura_common/publisher.hpp"

class SimNode {
public:
	SimNode(int instance);

	int handle_periodic_timer();

	static void heartbeat_timer_callback(EventSource *es);

private:
	Publisher gps_pub;

	int _lat;
	int _lon;
	float _angle;

};
