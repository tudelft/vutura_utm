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
		_armed(false)
	{

	}

	void uav_command(std::string command);
	void emit_heartbeat();
	void set_armed_state(bool armed);

	UdpSource mavlink_comm;

	Publisher gps_pub;
	Publisher uav_armed_pub;

	static void mavlink_comm_callback(EventSource* es);
	static void heartbeat_timer_callback(EventSource* es);
	static void uav_command_callback(EventSource* es);

private:
	bool _armed;
};
