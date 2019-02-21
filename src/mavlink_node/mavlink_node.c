#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>
#include <sys/timerfd.h>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/pubsub0/pub.h>

#include "mavlink_node.h"
#include <mavlink.h>

#include "proto/messages.pb-c.h"

#define BUFFER_LENGTH 2041

typedef struct mavlink_node_t
{
	const char* name;
	int uav_sock;
	int timerfd;
	struct sockaddr_in uav_addr;
	struct sockaddr_in loc_addr;
	uint8_t buf[BUFFER_LENGTH];
	nng_socket rep_fp_socket;
	int rep_fp_fd;
	nng_socket pub_gps_position_socket;
//	int pub_gps_position_fd;
	nng_socket pub_uav_heartbeat_socket;
//	int pub_uav_heartbeat_fd;
} mavlink_node_t;

void mavlink_node_timer_event(mavlink_node_t *node);
void mavlink_node_incoming_message(mavlink_node_t *node, mavlink_message_t *msg);
void mavlink_node_handle_request(mavlink_node_t *node, nng_msg *msg);

void mavlink_node_timer_event(mavlink_node_t *node)
{
	// Probably want to emit a heartbeat now
	mavlink_message_t msg;
	mavlink_msg_heartbeat_pack(255, 200, &msg, MAV_TYPE_GCS, MAV_AUTOPILOT_GENERIC, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);
	uint16_t len = mavlink_msg_to_send_buffer(node->buf, &msg);
	int bytes_sent = sendto(node->uav_sock, node->buf, len, 0, (struct sockaddr*)&node->uav_addr, sizeof(struct sockaddr_in));
	//printf("[%s] sent heartbeat %d bytes\n", node->name, bytes_sent);
}

void mavlink_node_incoming_message(mavlink_node_t *node, mavlink_message_t *msg)
{
	// The message is in the buffer
	//printf("[%s] incoming msg SYS: %d, COMP: %d, LEN: %d, MSGID: %d\n", node->name, msg->sysid, msg->compid, msg->len, msg->msgid);
	if (msg->msgid == MAVLINK_MSG_ID_GLOBAL_POSITION_INT) {
		mavlink_global_position_int_t global_pos;
		mavlink_msg_global_position_int_decode(msg, &global_pos);
		printf("[%s] got global position lat: %f lon: %f alt_msl: %f alt_agl: %f\n", node->name, global_pos.lat * 1e-7, global_pos.lon * 1e-7, global_pos.alt * 1e-3, global_pos.relative_alt * 1e-3);
		GPSMessage gps_message = GPSMESSAGE__INIT;
		uint16_t len;
		gps_message.timestamp = 0;
		gps_message.lat = global_pos.lat;
		gps_message.lon = global_pos.lon;
		gps_message.alt_msl = global_pos.alt;
		gps_message.alt_agl = global_pos.relative_alt;
		len = gpsmessage__get_packed_size(&gps_message);
		uint8_t buf[len];
		gpsmessage__pack(&gps_message, buf);

		nng_msg *msg;
		nng_msg_alloc(&msg, 0);
		nng_msg_append(msg, &buf, len);
		nng_sendmsg(node->pub_gps_position_socket, msg, 0);

		//printf("Writing %d serialized bytes\n", len);
	} else if (msg->msgid == MAVLINK_MSG_ID_HEARTBEAT) {
		printf("HB\n");
		// check if vehicle is armed
		mavlink_heartbeat_t hb;
		mavlink_msg_heartbeat_decode(msg, &hb);
		UavHeartbeat uavhb = UAV_HEARTBEAT__INIT;
		uint16_t len;
		uavhb.has_armed = true;
		uavhb.armed = hb.system_status == MAV_STATE_ACTIVE;
		len = uav_heartbeat__get_packed_size(&uavhb);
		uint8_t buf[len];
		uav_heartbeat__pack(&uavhb, buf);

		nng_msg *msg;
		nng_msg_alloc(&msg, 0);
		nng_msg_append(msg, buf, len);
		nng_sendmsg(node->pub_uav_heartbeat_socket, msg, 0);
	}
}

void mavlink_node_handle_request(mavlink_node_t *node, nng_msg *msg)
{
	// This function needs to free the msg when finished using it
	printf("Got %s\theader length: %lu\n", (char*)nng_msg_body(msg), nng_msg_header_len(msg));

	// send reply

	Flightplan fp = FLIGHTPLAN__INIT;
	fp.n_waypoint = 2;
	Flightplan__Waypoint first_waypoint = FLIGHTPLAN__WAYPOINT__INIT;
	Flightplan__Waypoint second_waypoint = FLIGHTPLAN__WAYPOINT__INIT;

	// Array of pointers to waypoints
	Flightplan__Waypoint *all_waypoints[fp.n_waypoint];
	all_waypoints[0] = &first_waypoint;
	all_waypoints[1] = &second_waypoint;
	fp.waypoint = all_waypoints;

	fp.waypoint[0]->lat = 10;
	fp.waypoint[0]->lon = 20;
	fp.waypoint[0]->alt = 30;
	fp.waypoint[0]->has_alt = true;

	fp.waypoint[1]->lat = -10;
	fp.waypoint[1]->lon = -123;
	fp.waypoint[1]->has_alt = false;

	unsigned len = flightplan__get_packed_size(&fp);
	nng_msg_realloc(msg, len);
	flightplan__pack(&fp, nng_msg_body(msg));

	// This function frees the msg
	nng_sendmsg(node->rep_fp_socket, msg, 0);
}

void
fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

int main(int argc, char **argv)
{
	mavlink_node_t node = {
		.name = "mavlink",
		.uav_sock = -1,
		.timerfd = -1,
		.rep_fp_fd = -1
//		.pub_gps_position_fd = -1
	};

	int rv;

	printf("Starting mavlink_node\n");

	char target_ip[100];
	strcpy(target_ip, "127.0.0.1");

	node.uav_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);


	// Listen to UAV
	memset(&node.loc_addr, 0, sizeof(node.loc_addr));
	node.loc_addr.sin_family = AF_INET;
	node.loc_addr.sin_addr.s_addr = INADDR_ANY;
	node.loc_addr.sin_port = htons(14551);

	if (bind(node.uav_sock, (struct sockaddr *)&node.loc_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("error bind failed");
		close(node.uav_sock);
		exit(EXIT_FAILURE);
	}

	// Configure socket for sending to UAV
	memset(&node.uav_addr, 0, sizeof(node.uav_addr));
	node.uav_addr.sin_family = AF_INET;
	node.uav_addr.sin_addr.s_addr = inet_addr(target_ip);
	node.uav_addr.sin_port = htons(14557);

	// Configure periodic timer for heartbeats
	node.timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
	struct timespec ts = {0};
	ts.tv_sec = 2;
	struct itimerspec its = {0};
	its.it_interval = ts;
	its.it_value = ts;
	if (timerfd_settime(node.timerfd, 0, &its, NULL) == -1) {
		perror("setting timer");
	}

	// Configure reply topic for flightplan request
	if ((rv = nng_rep0_open(&node.rep_fp_socket)) != 0) {
		fatal("nng_rep0_open", rv);
	}

	if ((rv = nng_listen(node.rep_fp_socket, "ipc:///tmp/fp.sock", NULL, NNG_FLAG_NONBLOCK))) {
		fatal("nng_listen", rv);
	}

	if ((rv = nng_getopt_int(node.rep_fp_socket, NNG_OPT_RECVFD, &node.rep_fp_fd)) != 0) {
		fatal("nng_getopt", rv);
	}

	// GPS position publishing
	if ((rv = nng_pub0_open(&node.pub_gps_position_socket)) != 0) {
		fatal("nng_pub0_open", rv);
	}

	if ((rv = nng_listen(node.pub_gps_position_socket, "ipc:///tmp/vutura/gps_position.sock", NULL, NNG_FLAG_NONBLOCK)) != 0) {
		fatal("nng_listen", rv);
	}

	// UAV Armed status
	if ((rv = nng_pub0_open(&node.pub_uav_heartbeat_socket)) != 0) {
		fatal("nng_pub0_open", rv);
	}

	if ((rv = nng_listen(node.pub_uav_heartbeat_socket, "ipc:///tmp/uav_armed.sock", NULL, NNG_FLAG_NONBLOCK)) != 0) {
		fatal("nng_listen", rv);
	}

	// Configure file descriptors for event listening
	const unsigned int num_fds = 2;
	struct pollfd fds[num_fds];
	fds[0].fd = node.uav_sock;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	fds[1].fd = node.timerfd;
	fds[1].events = POLLIN;
	fds[1].revents = 0;

	fds[2].fd = node.rep_fp_fd;
	fds[2].events = POLLIN;
	fds[2].revents = 0;

	bool should_exit = false;

	while (!should_exit) {
		int rv = poll(fds, num_fds, 3000);
		if (rv == 0) {
			printf("Timeout...\n");
			//should_exit = true;
			//break;
		}

		if (fds[0].revents & POLLIN) {
			memset(node.buf, 0, BUFFER_LENGTH);
			ssize_t recv_size = recv(fds[0].fd, (void *)node.buf, BUFFER_LENGTH, 0);
			if (recv_size > 0) {
				mavlink_message_t msg;
				mavlink_status_t status;

				for (ssize_t i = 0; i < recv_size; i++) {
					if (mavlink_parse_char(MAVLINK_COMM_0, node.buf[i], &msg, &status)) {
						mavlink_node_incoming_message(&node, &msg);
					}
				}
			}
		}

		if (fds[1].revents & POLLIN) {
			uint64_t num_timer_events;
			ssize_t recv_size = read(fds[1].fd, &num_timer_events, 8);
			(void) recv_size;
			//printf("timer %lu\n", recv_size);

			// Call timer event
			//printf("call mavlink_node_timer_event\n");
			mavlink_node_timer_event(&node);
			//printf("finished call\n\n");
			//should_exit = true;
		}

		if (fds[2].revents & POLLIN) {
			nng_msg *fp_msg;
			nng_recvmsg(node.rep_fp_socket, &fp_msg, NNG_FLAG_NONBLOCK);
			mavlink_node_handle_request(&node, fp_msg);
			if (fp_msg != NULL) {
				printf("NOT NULL");
				//free(fp_msg);
			}
			//should_exit = true;
		}
	}

	close(node.timerfd);
	close(node.uav_sock);
	nng_close(node.rep_fp_socket);

	return 0;
}
