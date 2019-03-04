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

#include "vutura_common/event_loop.hpp"
#include "vutura_common/udp_source.hpp"
#include "vutura_common/timer.hpp"
#include "vutura_common/subscription.hpp"

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
		UavHeartbeat uavhb;
		uavhb.set_armed(hb.system_status == MAV_STATE_ACTIVE);
		node->uav_armed_pub.publish(uavhb.SerializeAsString());
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

int main(int argc, char **argv)
{

	EventLoop event_loop;

	MavlinkNode node;
	event_loop.add(node.mavlink_comm);

	Timer heartbeat_timer(&node, 1000, node.heartbeat_timer_callback);
	event_loop.add(heartbeat_timer);

	Subscription uav_command_sub(&node, SOCK_PUBSUB_UAV_COMMAND, node.uav_command_callback);
	event_loop.add(uav_command_sub);

	event_loop.start();
}
