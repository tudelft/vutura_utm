#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>
#include <sys/timerfd.h>
#include <string>
#include <iostream>

#include "mavlink_node.h"
#include <mavlink.h>

#include "vutura_common/helper.hpp"
#include "vutura_common/event_loop.hpp"
#include "vutura_common/udp_source.hpp"
#include "vutura_common/timer.hpp"
#include "vutura_common/subscription.hpp"
#include "vutura_common/listener_replier.hpp"

#include "vutura_common.pb.cc"

#define BUFFER_LENGTH 2041

void mavlink_node_incoming_message(MavlinkNode *node, mavlink_message_t *msg);

void mavlink_node_incoming_message(MavlinkNode *node, mavlink_message_t *msg)
{
	// The message is in the buffer
	//printf("[%s] incoming msg SYS: %d, COMP: %d, LEN: %d, MSGID: %d\n", node->name, msg->sysid, msg->compid, msg->len, msg->msgid);
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

		node->gps_pub.publish(gps_message.SerializeAsString());

		//printf("Writing %d serialized bytes\n", len);
	} else if (msg->msgid == MAVLINK_MSG_ID_HEARTBEAT) {
//		printf("HB\n");
		// check if vehicle is armed
		mavlink_heartbeat_t hb;
		mavlink_msg_heartbeat_decode(msg, &hb);

		node->set_armed_state(hb.system_status == MAV_STATE_ACTIVE);
	}
}

void MavlinkNode::uav_command(std::string command)
{
	if (command == "start mission") {
		std::cout << "Should start mission now." << std::endl;
		mavlink_message_t msg;
		mavlink_msg_command_long_pack(MAVLINK_SYSTEM_ID, MAV_COMP_ID_SYSTEM_CONTROL, &msg, 1, 1, MAV_CMD_MISSION_START, 0, 0, 0, 0, 0, 0, 0, 0);
		uint16_t len = mavlink_msg_to_send_buffer(mavlink_comm.buf, &msg);
		mavlink_comm.send_buffer(len);

	} else {
		std::cout << "Received: " << command << std::endl;
	}
}

void MavlinkNode::emit_heartbeat()
{
	// Probably want to emit a heartbeat now
	mavlink_message_t msg;
	mavlink_msg_heartbeat_pack(255, 200, &msg, MAV_TYPE_GCS, MAV_AUTOPILOT_GENERIC, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);
	uint16_t len = mavlink_msg_to_send_buffer(mavlink_comm.buf, &msg);
	mavlink_comm.send_buffer(len);
	//printf("[%s] sent heartbeat %d bytes\n", node->name, bytes_sent);
}

void MavlinkNode::avoidance_velocity_vector(bool avoid, float vx, float vy, float vz)
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
	offboard_target.vx = vx;
	offboard_target.vy = vy;
	offboard_target.vz = vz;
	offboard_target.yaw = std::atan2(offboard_target.vy, offboard_target.vx);

	mavlink_message_t msg;
	mavlink_msg_set_position_target_local_ned_encode(255, 200, &msg, &offboard_target);
	uint16_t len = mavlink_msg_to_send_buffer(mavlink_comm.buf, &msg);
	mavlink_comm.send_buffer(len);

	// Enable avoidance mode
	if (_avoiding != avoid && avoid) {
		enable_offboard(true);
		_avoiding = avoid;
	}

}

void MavlinkNode::set_armed_state(bool armed)
{
	if (armed != _armed) {
		// update state and publish
		_armed = armed;

		UavHeartbeat uavhb;
		uavhb.set_armed(_armed);
		uav_armed_pub.publish(uavhb.SerializeAsString());

	}
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
	uint16_t len = mavlink_msg_to_send_buffer(mavlink_comm.buf, &msg);
	mavlink_comm.send_buffer(len);
}

void MavlinkNode::mavlink_comm_callback(EventSource *es) {
	UdpSource *udp = static_cast<UdpSource*>(es);
	MavlinkNode *node = static_cast<MavlinkNode*>(udp->_target_object);
	memset(udp->buf, 0, BUFFER_LENGTH);
	ssize_t recv_size = recv(udp->_fd, (void *)udp->buf, BUFFER_LENGTH, 0);
	if (recv_size > 0) {
		mavlink_message_t msg;
		mavlink_status_t status;

		for (ssize_t i = 0; i < recv_size; i++) {
			if (mavlink_parse_char(MAVLINK_COMM_0, udp->buf[i], &msg, &status)) {
				mavlink_node_incoming_message(node, &msg);
			}
		}
	}
}

void MavlinkNode::heartbeat_timer_callback(EventSource *es) {
	// read the timer
	uint64_t num_timer_events;
	ssize_t recv_size = read(es->_fd, &num_timer_events, 8);
	(void) recv_size;

	Timer *tim = static_cast<Timer*>(es);
	MavlinkNode *node = static_cast<MavlinkNode*>(tim->_target_object);
	node->emit_heartbeat();
}

void MavlinkNode::uav_command_callback(EventSource *es)
{
	Subscription *rep = static_cast<Subscription*>(es);
	MavlinkNode *node = static_cast<MavlinkNode*>(rep->_target_object);
	std::string message = rep->get_message();

	node->uav_command(message);
}

void MavlinkNode::avoidance_command_callback(EventSource *es)
{
	ListenerReplier *rep = static_cast<ListenerReplier*>(es);
	MavlinkNode *node = static_cast<MavlinkNode*>(rep->_target_object);
	std::string message = rep->get_message();

	// Assume that this was an avoidance velocity message
	AvoidanceVelocity avoidance_msg;
	std::string response = "OK";
	try {
		bool success = avoidance_msg.ParseFromString(message);
		if (!success) {
			response = "NOTOK";
		} else {
//			std::cout << "vx: " << std::to_string(avoidance_msg.vx()) << std::endl;
//			std::cout << "vy: " << std::to_string(avoidance_msg.vy()) << std::endl;
//			std::cout << "vz: " << std::to_string(avoidance_msg.vz()) << std::endl;
		}
	} catch (...) {
		std::cerr << "Error parsing avoidance command" << std::endl;
		response = "NOTOK";
	}

	rep->send_response(response);

	// do something with it
	node->avoidance_velocity_vector(avoidance_msg.avoid(), 0.001 * avoidance_msg.vx(), 0.001 * avoidance_msg.vy(), 0.001 * avoidance_msg.vz());

}

int main(int argc, char **argv)
{
	int instance = 0;
	if (argc > 1) {
		instance = atoi(argv[1]);
	}
	std::cout << "Instance " << std::to_string(instance) << std::endl;

	EventLoop event_loop;

	MavlinkNode node(instance);
	event_loop.add(node.mavlink_comm);

	Timer heartbeat_timer(&node, 1000, node.heartbeat_timer_callback);
	event_loop.add(heartbeat_timer);

	Subscription uav_command_sub(&node, socket_name(SOCK_PUBSUB_UAV_COMMAND, instance), node.uav_command_callback);
	event_loop.add(uav_command_sub);

	ListenerReplier avoidance_rep(&node, socket_name(SOCK_REQREP_AVOIDANCE_COMMAND, instance), node.avoidance_command_callback);
	event_loop.add(avoidance_rep);

	event_loop.start();
}
