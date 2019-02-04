#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>

#include "mavlink_node.h"
#include <mavlink.h>

#define BUFFER_LENGTH 2041

typedef struct mavlink_node_t
{
	int uav_sock;
} mavlink_node_t;


int main(int argc, char **argv)
{
	mavlink_node_t node = {
		.uav_sock = -1
	};

	printf("Starting mavlink_node\n");

	char target_ip[100];
	strcpy(target_ip, "127.0.0.1");

	node.uav_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in uav_addr;
	struct sockaddr_in loc_addr;

	uint8_t buf[BUFFER_LENGTH];

	// Listen to UAV
	memset(&loc_addr, 0, sizeof(loc_addr));
	loc_addr.sin_family = AF_INET;
	loc_addr.sin_addr.s_addr = INADDR_ANY;
	loc_addr.sin_port = htons(14551);

	if (bind(node.uav_sock, (struct sockaddr *)&loc_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("error bind failed");
		close(node.uav_sock);
		exit(EXIT_FAILURE);
	}

	// Configure socket for sending to UAV
	memset(&uav_addr, 0, sizeof(uav_addr));
	uav_addr.sin_family = AF_INET;
	uav_addr.sin_addr.s_addr = inet_addr(target_ip);
	uav_addr.sin_port = htons(14550);

	// Configure file descriptors for event listening
	struct pollfd fds[1];
	fds[0].fd = node.uav_sock;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	bool should_exit = false;

	// Send first UDP message
	mavlink_message_t msg;
	mavlink_msg_heartbeat_pack(1, 200, &msg, MAV_TYPE_HELICOPTER, MAV_AUTOPILOT_GENERIC, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);
	uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
	int bytes_sent = sendto(node.uav_sock, buf, len, 0, (struct sockaddr*)&uav_addr, sizeof(struct sockaddr_in));
	printf("Sent %d bytes\n", bytes_sent);

	while (!should_exit) {
		int rv = poll(fds, 1, 3000);
		if (rv == 0) {
			printf("Timeout...\n");
			//should_exit = true;
			//break;
		}

		if (fds[0].revents == POLLIN) {
			ssize_t recv_size = recv(node.uav_sock, (void *)buf, BUFFER_LENGTH, 0);
			printf("Got UDP size %lu\n", recv_size);
		}
	}
}
