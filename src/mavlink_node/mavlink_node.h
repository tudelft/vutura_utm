#pragma once
#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "vutura_common/udp_source.hpp"
#include "vutura_common/publisher.hpp"

class MavlinkNode {
public:
	MavlinkNode(int instance) :
		mavlink_comm(this, MAVLINK_IP, MAVLINK_PORT, mavlink_comm_callback),
		gps_pub(socket_name(SOCK_PUBSUB_GPS_POSITION, instance)),
		uav_armed_pub(socket_name(SOCK_PUBSUB_UAV_STATUS, instance)),
		_armed(false),
		_guided_mode(false),
		_avoiding(false)
	{

	}

	void uav_command(std::string command);
	void emit_heartbeat();
        void avoidance_velocity_vector(bool avoid, float vn, float ve, float vd);
	void set_armed_state(bool armed);
	void set_guided_state(bool guided_mode_enabled);
	void enable_offboard(bool offboard);

	UdpSource mavlink_comm;

	Publisher gps_pub;
	Publisher uav_armed_pub;

	static void mavlink_comm_callback(EventSource* es);
	static void heartbeat_timer_callback(EventSource* es);
	static void uav_command_callback(EventSource* es);
	static void avoidance_command_callback(EventSource* es);

private:
	bool _armed;
	bool _guided_mode;
	bool _avoiding;
};
