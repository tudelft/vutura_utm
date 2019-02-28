#pragma once
#include "vutura_common/udp_source.hpp"
#include "vutura_common/publisher.hpp"

class MavlinkNode {
public:
	MavlinkNode() :
		mavlink_comm(this, "127.0.0.1", 14557, mavlink_comm_callback),
		gps_pub("ipc:///tmp/gps_position.sock"),
		uav_armed_pub("ipc:///tmp/uav_armed.sock")
	{

	}

	void uav_command(std::string command);
	void emit_heartbeat();

	UdpSource mavlink_comm;

	Publisher gps_pub;
	Publisher uav_armed_pub;

	static void mavlink_comm_callback(EventSource* es);
	static void heartbeat_timer_callback(EventSource* es);
	static void uav_command_callback(EventSource* es);

};
