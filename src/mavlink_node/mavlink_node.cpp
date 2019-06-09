#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <iostream>

#include "mavlink_node.hpp"

#include "vutura_common/helper.hpp"

#include "vutura_common.pb.h"

using namespace std::placeholders;

MavlinkNode::MavlinkNode(int instance) :
	_instance(instance),
	_armed(false),
	_guided_mode(false),
	_avoiding(false),
	_arming_delay(-1),
	_aborted(false),
	_heartbeat_timer(this),
	_mavlink_comm(this),
	_uav_command_sub(this),
	_avoidance_rep(this),
	_direct_mav_command_sub(this),
	_gps_pub(socket_name(SOCK_PUBSUB_GPS_POSITION, instance)),
	_gps_json_pub(socket_name(SOCK_PUBSUB_GPS_JSON, instance)),
	_uav_armed_pub(socket_name(SOCK_PUBSUB_UAV_STATUS, instance))
{

}

MavlinkNode::~MavlinkNode()
{

}

void MavlinkNode::init()
{
	_heartbeat_timer.set_timeout_callback(std::bind(&MavlinkNode::periodic, this));
	_heartbeat_timer.start_periodic(1000); // be careful with changing this timer, is is also used for arming delay

	_mavlink_comm.set_receive_callback(std::bind(&MavlinkNode::handle_udp_packet, this, _1));
	_mavlink_comm.connect(MAVLINK_IP, MAVLINK_PORT, 14551);

	_uav_command_sub.set_receive_callback(std::bind(&MavlinkNode::uav_command, this, _1));
	_uav_command_sub.subscribe(socket_name(SOCK_PUBSUB_UAV_COMMAND, _instance));

	_avoidance_rep.set_receive_callback(std::bind(&MavlinkNode::handle_avoidance_command, this, _1, _2));
	_avoidance_rep.listen(socket_name(SOCK_REQREP_AVOIDANCE_COMMAND, _instance));

	// same as uav command
	_direct_mav_command_sub.set_receive_callback(std::bind(&MavlinkNode::uav_command, this, _1));
	_direct_mav_command_sub.subscribe(socket_name(SOCK_SUB_DIRECT_MAV_COMMAND, _instance));
}

void MavlinkNode::uav_command(std::string command)
{
	if (command == "start mission") {
		std::cout << "Start timeout for mission." << std::endl;
		_arming_delay = 0;
		_aborted = false;

	} else if (command == "abort") {
		std::cout << "Stop mission countdown sequence" << std::endl;
		if (_arming_delay >= 0) {
			std::cout << "ABORT BEEP" << std::endl;
		} else {
			std::cout << "TOO LATE" << std::endl;
		}
		_arming_delay = -1;
		_aborted = true;

	} else if (command == "abort confirmed") {
		std::cout << "abort confirmed" << std::endl;
		_aborted = false;

	} else if (command == "HB") {
		// do nothing

	} else if (command == "kill kill kill") {
		send_command_terminate();

	} else if (command == "pause") {
		send_command_hold();

	} else if (command == "continue") {
		send_command_continue();

	} else if (command == "land here") {
		send_command_land();

	} else {
		std::cout << "Received: " << command << std::endl;
	}
}

void MavlinkNode::periodic()
{
	emit_heartbeat();

	// pre-arming timer
	if (_arming_delay >= 0) {
		_arming_delay++;
		// make sound
		std::cout << "Play beep " << std::to_string(_arming_delay) << std::endl;
		if (_arming_delay < 16) {
			play_tune(14);
		} else if (_arming_delay == 16) {
			play_tune(8);
		} else if (_arming_delay >= 19) {
			play_tune(12);
		}
	}

	if (_arming_delay == 20) {
		start_mission();
		_arming_delay = -1;
	}
}

void MavlinkNode::emit_heartbeat()
{
	mavlink_message_t msg;
	mavlink_msg_heartbeat_pack(255, 200, &msg, MAV_TYPE_GCS, MAV_AUTOPILOT_GENERIC, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);
	uint16_t len = mavlink_msg_to_send_buffer(_buffer, &msg);

	std::string packet((char*)_buffer, len);
	_mavlink_comm.send_packet(packet);

	//printf("[%s] sent heartbeat %d bytes\n", node->name, bytes_sent);
}

void MavlinkNode::handle_udp_packet(std::string packet)
{
	const char* packet_buffer = packet.data();

	mavlink_message_t msg;
	mavlink_status_t status;

	for (ssize_t i = 0; i < packet.length(); i++) {
		if (mavlink_parse_char(MAVLINK_COMM_0, packet_buffer[i], &msg, &status)) {
			handle_mavlink_message(&msg);
		}
	}
}

void MavlinkNode::handle_mavlink_message(mavlink_message_t *msg)
{
	//printf("[mavlink_node] incoming msg SYS: %d, COMP: %d, LEN: %d, MSGID: %d\n", msg->sysid, msg->compid, msg->len, msg->msgid);

	if (msg->sysid != 1 && msg->compid != 1) {
		// message not from MAV
		return;
	}

	if (msg->msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
		mavlink_global_position_int_t global_pos;
		mavlink_msg_global_position_int_decode(msg, &global_pos);

		//printf("[mavlink_node] got global position lat: %f lon: %f alt_msl: %f alt_agl: %f\n", global_pos.lat * 1e-7, global_pos.lon * 1e-7, global_pos.alt * 1e-3, global_pos.relative_alt * 1e-3);
		GPSMessage gps_message;
		uint16_t len;
		gps_message.set_timestamp(0);
		gps_message.set_lat(global_pos.lat);
		gps_message.set_lon(global_pos.lon);
		gps_message.set_alt_msl(global_pos.alt);
		gps_message.set_alt_agl(global_pos.relative_alt);

		gps_message.set_vn(global_pos.vx * 10);
		gps_message.set_ve(global_pos.vy * 10);
		gps_message.set_vd(global_pos.vz * 10);

		// send as json string
		std::string gps_json_string = "{";
		gps_json_string += "\"lat\": " + std::to_string(global_pos.lat) + ", ";
		gps_json_string += "\"lon\": " + std::to_string(global_pos.lon) + ", ";
		gps_json_string += "\"hdg\": " + std::to_string(global_pos.hdg) + ", ";
		gps_json_string += "\"height\": " + std::to_string(global_pos.relative_alt) + "}";

		_gps_json_pub.publish(gps_json_string);

		static unsigned int skip_counter = 0;
		if (skip_counter <= 20) {
			skip_counter++;
			return;
		}
		skip_counter = 0;

		std::string gps_message_string = gps_message.SerializeAsString();
		_gps_pub.publish(gps_message_string);


	} else if (msg->msgid == MAVLINK_MSG_ID_HEARTBEAT) {
		// std::cout << "HB received" << std::endl;
		// check if vehicle is armed
		mavlink_heartbeat_t hb;
		mavlink_msg_heartbeat_decode(msg, &hb);

		set_armed_state(hb.system_status == MAV_STATE_ACTIVE);
		set_guided_state(hb.base_mode & MAV_MODE_FLAG_GUIDED_ENABLED);
	}
}

void MavlinkNode::handle_avoidance_command(std::string request, std::string &reply)
{
	 //Assume that this was an avoidance velocity message
	AvoidanceVelocity avoidance_msg;
	try {
		bool success = avoidance_msg.ParseFromString(request);
		if (success) {
			reply = "OK";
//			std::cout << "vx: " << std::to_string(avoidance_msg.vx()) << std::endl;
//			std::cout << "vy: " << std::to_string(avoidance_msg.vy()) << std::endl;
//			std::cout << "vz: " << std::to_string(avoidance_msg.vz()) << std::endl;
		}
	} catch (...) {
		std::cerr << "Error parsing avoidance command" << std::endl;
	}

	// do something with it
	avoidance_velocity_vector(avoidance_msg.avoid(), 0.001 * avoidance_msg.vn(), 0.001 * avoidance_msg.ve(), 0.001 * avoidance_msg.vd());

}

void MavlinkNode::avoidance_velocity_vector(bool avoid, float vn, float ve, float vd)
{
	// Disable avoidance mode
	if (_avoiding != avoid && !avoid) {
		enable_offboard(false);
		_avoiding = avoid;
		return;
	}

	// Also try sending a command to use in offboard mode
	mavlink_set_position_target_local_ned_t offboard_target;
	offboard_target.time_boot_ms = 0;
	offboard_target.target_component = 1;
	offboard_target.target_system = 1;
	offboard_target.coordinate_frame = MAV_FRAME_LOCAL_NED;
	offboard_target.type_mask =
			POSITION_TARGET_TYPEMASK_X_IGNORE |
			POSITION_TARGET_TYPEMASK_Y_IGNORE |
			POSITION_TARGET_TYPEMASK_Z_IGNORE |
			POSITION_TARGET_TYPEMASK_AX_IGNORE |
			POSITION_TARGET_TYPEMASK_AY_IGNORE |
			POSITION_TARGET_TYPEMASK_AZ_IGNORE |
			POSITION_TARGET_TYPEMASK_YAW_RATE_IGNORE;
	offboard_target.vx = vn;
	offboard_target.vy = ve;
	offboard_target.vz = vd;
	offboard_target.yaw = atan2(offboard_target.vy, offboard_target.vx);

	mavlink_message_t msg;
	mavlink_msg_set_position_target_local_ned_encode(255, 200, &msg, &offboard_target);
	uint16_t len = mavlink_msg_to_send_buffer(_buffer, &msg);

	std::string packet((char*)_buffer, len);
	_mavlink_comm.send_packet(packet);

	// Enable avoidance mode, only allowed during missions (guided mode)
	if (_guided_mode) {
		if (_avoiding != avoid && avoid) {
			enable_offboard(true);
			_avoiding = avoid;
		}
	}

}

void MavlinkNode::set_armed_state(bool armed)
{

	// update state and publish
	_armed = armed;

	UavHeartbeat uavhb;
	uavhb.set_armed(_armed);
	uavhb.set_aborted(_aborted);
	std::string message_string = uavhb.SerializeAsString();
	_uav_armed_pub.publish(message_string);
}

void MavlinkNode::set_guided_state(bool guided_mode_enabled)
{
	_guided_mode = guided_mode_enabled;
//	if (_guided_mode) {
//		std::cout << "GUIDED" << std::endl;
//	} else {
//		std::cout << "Not guided" << std::endl;
//	}
}

void MavlinkNode::enable_offboard(bool offboard)
{
	mavlink_message_t msg;
	mavlink_command_long_t command_msg;
	command_msg.target_system = 1;
	command_msg.target_component = 1;
	command_msg.command = MAV_CMD_NAV_GUIDED_ENABLE;

	if (offboard) {
		// mavlink command to enable offboard
		command_msg.param1 = 1;
	} else {
		command_msg.param1 = 0;
	}

	std::cout << "Sending offboard mode " << std::to_string(command_msg.param1) << std::endl;

	mavlink_msg_command_long_encode(255, 200, &msg, &command_msg);
	uint16_t len = mavlink_msg_to_send_buffer(_buffer, &msg);

	std::string packet((char*)_buffer, len);
	_mavlink_comm.send_packet(packet);
}

void MavlinkNode::play_tune(int tune_nr)
{
	// try sending beep command
	char buf[70] = {0};
	ssize_t buflen = sprintf(buf, "tune_control play -t %d\n", tune_nr);

	std::cout << "Send \"" << buf << "\"" << std::endl;
//	memcpy(buf, command, sizeof(command));
	mavlink_message_t msg;
	mavlink_msg_serial_control_pack(255, 200, &msg, SERIAL_CONTROL_DEV_SHELL, /* SERIAL_CONTROL_FLAG_EXCLUSIVE | */ SERIAL_CONTROL_FLAG_RESPOND, 0, 0, buflen, (uint8_t*)buf);
	uint16_t len = mavlink_msg_to_send_buffer(_buffer, &msg);

	std::string packet((char*)_buffer, len);
	_mavlink_comm.send_packet(packet);

}

void MavlinkNode::start_mission()
{
	std::cout << "Start mission command!" << std::endl;
	mavlink_message_t msg;
	mavlink_msg_command_long_pack(MAVLINK_SYSTEM_ID, MAV_COMP_ID_SYSTEM_CONTROL, &msg, 1, 1, MAV_CMD_MISSION_START, 0, 0, 0, 0, 0, 0, 0, 0);
	uint16_t len = mavlink_msg_to_send_buffer(_buffer, &msg);
	std::string packet((char*)_buffer, len);
	_mavlink_comm.send_packet(packet);

}

void MavlinkNode::send_mavlink_message(mavlink_message_t *msg)
{
	uint16_t len = mavlink_msg_to_send_buffer(_buffer, msg);
	std::string packet((char*)_buffer, len);
	_mavlink_comm.send_packet(packet);
}

void MavlinkNode::send_command_hold()
{
	std::cout << "pause" << std::endl;
	mavlink_message_t msg;
	mavlink_msg_command_long_pack(MAVLINK_SYSTEM_ID, MAV_COMP_ID_SYSTEM_CONTROL, &msg, 1, 1, MAV_CMD_DO_SET_MODE, 0,
				      MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_AUTO_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED,
				      MAV_MODE_FLAG_AUTO_ENABLED, 3, 0, 0, 0, 0);
	return send_mavlink_message(&msg);
}

void MavlinkNode::send_command_continue()
{
	// 29 4 4
	std::cout << "continue" << std::endl;
	mavlink_message_t msg;
	mavlink_msg_command_long_pack(MAVLINK_SYSTEM_ID, MAV_COMP_ID_SYSTEM_CONTROL, &msg, 1, 1, MAV_CMD_DO_SET_MODE, 0,
				      MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_AUTO_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED,
				      MAV_MODE_FLAG_AUTO_ENABLED, 4, 0, 0, 0, 0);
	return send_mavlink_message(&msg);
}

void MavlinkNode::send_command_land()
{
	// 157 4 6
	std::cout << "land" << std::endl;
	mavlink_message_t msg;
	mavlink_msg_command_long_pack(MAVLINK_SYSTEM_ID, MAV_COMP_ID_SYSTEM_CONTROL, &msg, 1, 1, MAV_CMD_DO_SET_MODE, 0,
				      MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_AUTO_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED,
				      MAV_MODE_FLAG_AUTO_ENABLED, 6, 0, 0, 0, 0);
	return send_mavlink_message(&msg);
}

void MavlinkNode::send_command_disarm()
{
	std::cout << "CMD DISARM" << std::endl;
	mavlink_message_t msg;
	mavlink_msg_command_long_pack(MAVLINK_SYSTEM_ID, MAV_COMP_ID_SYSTEM_CONTROL, &msg, 1, 1, MAV_CMD_COMPONENT_ARM_DISARM, 0, 0, 0, 0, 0, 0, 0, 0);
	return send_mavlink_message(&msg);
}

void MavlinkNode::send_command_terminate()
{
	std::cout << "CMD TERMINATE" << std::endl;
	mavlink_message_t msg;
	mavlink_msg_command_long_pack(MAVLINK_SYSTEM_ID, MAV_COMP_ID_SYSTEM_CONTROL, &msg, 1, 1, MAV_CMD_DO_FLIGHTTERMINATION, 0, 1.0f, 0, 0, 0, 0, 0, 0);
	return send_mavlink_message(&msg);
}
