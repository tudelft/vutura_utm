#ifndef SIM_NODE_HPP
#define SIM_NODE_HPP

//#include "vutura_common/config.hpp"
//#include "vutura_common/udp_source.hpp"
//#include "vutura_common/publisher.hpp"
#include "nng_event_loop/event_loop.hpp"
#include "nng_event_loop/publisher.hpp"
#include "nng_event_loop/timer.hpp"

class SimNode : public EventLoop {
public:
	SimNode(int instance);
	~SimNode();

	void init();
	int handle_periodic_timer();

private:
	Timer _periodic_timer;
	Publisher _gps_pub;

	int _lat;
	int _lon;
	float _angle;

};

#endif // SIM_NODE_HPP
