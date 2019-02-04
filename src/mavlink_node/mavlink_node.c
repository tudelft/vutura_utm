#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>
#include <sys/timerfd.h>

#include "mavlink_node.h"
#include <mavlink.h>

#define BUFFER_LENGTH 2041

typedef struct mavlink_node_t
{
	const char* name;
	int uav_sock;
	int timerfd;
	struct sockaddr_in uav_addr;
	struct sockaddr_in loc_addr;
	uint8_t buf[BUFFER_LENGTH];
} mavlink_node_t;

void mavlink_node_timer_event(mavlink_node_t *node);
void mavlink_node_incoming_message(mavlink_node_t *node, mavlink_message_t *msg);

void mavlink_node_timer_event(mavlink_node_t *node)
{
	// Probably want to emit a heartbeat now
	mavlink_message_t msg;
	mavlink_msg_heartbeat_pack(1, 200, &msg, MAV_TYPE_HELICOPTER, MAV_AUTOPILOT_GENERIC, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);
	uint16_t len = mavlink_msg_to_send_buffer(node->buf, &msg);
	int bytes_sent = sendto(node->uav_sock, node->buf, len, 0, (struct sockaddr*)&node->uav_addr, sizeof(struct sockaddr_in));
	printf("[%s] sent heartbeat %d bytes\n", node->name, bytes_sent);
}

void mavlink_node_incoming_message(mavlink_node_t *node, mavlink_message_t *msg)
{
	// The message is in the buffer
	printf("[%s] incoming msg SYS: %d, COMP: %d, LEN: %d, MSGID: %d\n", node->name, msg->sysid, msg->compid, msg->len, msg->msgid);
}

int main(int argc, char **argv)
{
	mavlink_node_t node = {
		.name = "mavlink",
		.uav_sock = -1,
		.timerfd = -1
	};

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
	node.uav_addr.sin_port = htons(14550);

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

	// Configure file descriptors for event listening
	const unsigned int num_fds = 2;
	struct pollfd fds[num_fds];
	fds[0].fd = node.uav_sock;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	fds[1].fd = node.timerfd;
	fds[1].events = POLLIN;
	fds[1].revents = 0;

	bool should_exit = false;

	while (!should_exit) {
		int rv = poll(fds, num_fds, 3000);
		if (rv == 0) {
			printf("Timeout...\n");
			//should_exit = true;
			//break;
		}

		if (fds[0].revents == POLLIN) {
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

		if (fds[1].revents == POLLIN) {
			uint64_t num_timer_events;
			ssize_t recv_size = read(fds[1].fd, &num_timer_events, 8);
			(void) recv_size;
			//printf("timer %lu\n", recv_size);

			// Call timer event
			//printf("call mavlink_node_timer_event\n");
			mavlink_node_timer_event(&node);
			//printf("finished call\n\n");
		}
	}
}
