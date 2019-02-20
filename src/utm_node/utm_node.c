#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/protocol/pubsub0/sub.h>

#include "proto/messages.pb-c.h"

typedef struct utm_node_t
{
	const char* name;
	nng_socket req_fp_sock;
	int req_fp_fd;
	nng_socket sub_uav_armed_socket;
	int sub_uav_armed_fd;
} utm_node_t;

void utm_node_handle_uav_armed(utm_node_t *node, nng_msg *msg);

void utm_node_handle_uav_armed(utm_node_t *node, nng_msg *msg)
{
//	printf("uav armed message\n");
	// unpack
	UavHeartbeat *uavhb_msg;
	uavhb_msg = uav_heartbeat__unpack(NULL, nng_msg_len(msg), nng_msg_body(msg));
	if (uavhb_msg == NULL) {
		printf("unpacking failed\n");
	}

	if (uavhb_msg->has_armed) {
		printf("Armed: %s\n", uavhb_msg->armed ? "yes" : "no");
	}
}

void
fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

int main(int argc, char **argv)
{
	utm_node_t node = {
		.name = "utm",
		.req_fp_sock = -1,
		.req_fp_fd = -1,
		.sub_uav_armed_fd = -1
	};

	int rv;

	/*
	// Configure reply topic for flightplan request
	if ((rv = nng_req0_open(&node.req_fp_sock)) != 0) {
		fatal("nng_req0_open", rv);
	}

	if ((rv = nng_dial(node.req_fp_sock, "ipc:///tmp/fp.sock", NULL, NNG_FLAG_NONBLOCK))) {
		fatal("nng_dial", rv);
	}

	if ((rv = nng_getopt_int(node.req_fp_sock, NNG_OPT_SENDFD, &node.req_fp_fd))) {
		fatal("nng_getopt", rv);
	}
*/
	// Subscribe to uav armed status
	if ((rv = nng_sub0_open(&node.sub_uav_armed_socket))) {
		fatal("nng_sub0_open", rv);
	}

	if ((rv = nng_setopt(node.sub_uav_armed_socket, NNG_OPT_SUB_SUBSCRIBE, "", 0)) != 0) {
		fatal("nng_setopt", rv);
	}

	if ((rv = nng_dial(node.sub_uav_armed_socket, "ipc:///tmp/uav_armed.sock", NULL, NNG_FLAG_NONBLOCK))) {
		fatal("nng_dial", rv);
	}

	if ((rv = nng_getopt_int(node.sub_uav_armed_socket, NNG_OPT_RECVFD, &node.sub_uav_armed_fd))) {
		fatal("nng_getopt", rv);
	}

	printf("receivefd: %d\n", node.sub_uav_armed_fd);

	// Configure file descriptors for event listening
	const unsigned int num_fds = 1;
	struct pollfd fds[num_fds];

	fds[0].fd = node.sub_uav_armed_fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	bool should_exit = false;

	/*
	// send one flightplan request now
	nng_msg *nngmsg;
	if ((rv = nng_msg_alloc(&nngmsg, 0)) != 0) {
		fatal("nng_msg_alloc", rv);
	}

	nng_sendmsg(node.req_fp_sock, nngmsg, 0);

	// Receive the flightplan
	nng_recvmsg(node.req_fp_sock, &nngmsg, 0);
	Flightplan *fp;
	fp = flightplan__unpack(NULL, nng_msg_len(nngmsg), nng_msg_body(nngmsg));
	printf("# waypoints: %lu\n", fp->n_waypoint);
	for (ssize_t i = 0; i < fp->n_waypoint; i++) {
		if (fp->waypoint[i]->has_alt) {
			printf("wp #%lu: lat: %i\tlon: %i\talt: %i\n", i, fp->waypoint[i]->lat, fp->waypoint[i]->lon, fp->waypoint[i]->alt);

		} else {
			printf("wp #%lu: lat: %i\tlon: %i\talt: --\n", i, fp->waypoint[i]->lat, fp->waypoint[i]->lon);

		}
	}

	flightplan__free_unpacked(fp, NULL);
	nng_msg_free(nngmsg);

	*/

	while (!should_exit) {
		int rv = poll(fds, num_fds, 3000);
		if (rv == 0) {
			printf("Timeout...\n");
			//should_exit = true;
			//break;
		}

		if (fds[0].revents & POLLIN) {
			nng_msg *uav_armed_msg;
			nng_recvmsg(node.sub_uav_armed_socket, &uav_armed_msg, 0);
			utm_node_handle_uav_armed(&node, uav_armed_msg);
			nng_msg_free(uav_armed_msg);
		}
	}
}
