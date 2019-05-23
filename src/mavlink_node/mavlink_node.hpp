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
	void periodic();
	void emit_heartbeat();
	void handle_udp_packet(std::string packet);
	void handle_mavlink_message(mavlink_message_t *msg);
	void handle_avoidance_command(std::string request, std::string& reply);

        void avoidance_velocity_vector(bool avoid, float vn, float ve, float vd);
	void set_armed_state(bool armed);
	void set_guided_state(bool guided_mode_enabled);
	void enable_offboard(bool offboard);

private:

	void play_tune(int tune_nr);
	void start_mission();
	void send_mavlink_message(mavlink_message_t *msg);
	void send_command_hold();
	void send_command_continue();
	void send_command_land();
	void send_command_disarm();
	int _instance;
	bool _armed;
	bool _guided_mode;
	bool _avoiding;
	int _arming_delay;
	bool _aborted;

	Timer _heartbeat_timer;
	UdpSource _mavlink_comm;
	Subscriber _uav_command_sub;
	Replier _avoidance_rep;
	Subscriber _direct_mav_command_sub;

	Publisher _gps_pub;
	Publisher _gps_json_pub;
	Publisher _uav_armed_pub;

	uint8_t _buffer[2041];

};

#endif // MAVLINK_NODE_HPP
