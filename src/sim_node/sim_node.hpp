#ifndef SIM_NODE_HPP
#define SIM_NODE_HPP

//#include "vutura_common/config.hpp"
//#include "vutura_common/udp_source.hpp"
//#include "vutura_common/publisher.hpp"
#include "nng_event_loop/event_loop.hpp"
#include "nng_event_loop/publisher.hpp"
#include "nng_event_loop/timer.hpp"

struct position_params
{
    double latd;
    double lat;
    double lond;
    double lon;
    double alt;
    double coslat;
    double sinlat;
    double r;
};

struct latdlond
{
	double latd;
	double lond;
};

struct n_e_coordinate
{
	double north;
	double east;
};

class SimNode : public EventLoop {
public:
	SimNode(int instance);
	~SimNode();

	void init();
	int handle_periodic_timer();

private:
	Timer _periodic_timer;
	Publisher _gps_pub;

	position_params _pos;
	float _angle;

};

#endif // SIM_NODE_HPP
