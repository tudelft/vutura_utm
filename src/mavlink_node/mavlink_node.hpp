#ifndef MAVLINK_NODE_HPP
#define MAVLINK_NODE_HPP

#include "mavlink.h"

#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "nng_event_loop/timer.hpp"
#include "nng_event_loop/subscriber.hpp"
#include "nng_event_loop/replier.hpp"
#include "nng_event_loop/udp_source.hpp"
#include "nng_event_loop/publisher.hpp"

class MavlinkNode : public EventLoop {
public:
	MavlinkNode(int instance);
	~MavlinkNode();


	void init();
	void uav_command(std::string command);
	void emit_heartbeat();
	void handle_udp_packet(std::string packet);
	void handle_mavlink_message(mavlink_message_t *msg);
	void handle_avoidance_command(std::string request, std::string& reply);

        void avoidance_velocity_vector(bool avoid, float vn, float ve, float vd);
	void set_armed_state(bool armed);
	void set_guided_state(bool guided_mode_enabled);
	void enable_offboard(bool offboard);

private:
	int _instance;
	bool _armed;
	bool _guided_mode;
	bool _avoiding;

	Timer _heartbeat_timer;
	UdpSource _mavlink_comm;
	Subscriber _uav_command_sub;
	Replier _avoidance_rep;

	Publisher _gps_pub;
	Publisher _uav_armed_pub;

	uint8_t _buffer[2041];

};

#endif // MAVLINK_NODE_HPP
