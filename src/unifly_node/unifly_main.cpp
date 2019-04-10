#include <unistd.h>


#include "unifly_config.h"
#include "vutura_common/config.hpp"
#include "vutura_common/vutura_common.pb.h"
#include "vutura_common/listener_replier.hpp"
#include "vutura_common/subscription.hpp"
#include "vutura_common/timer.hpp"
#include "vutura_common/publisher.hpp"
#include "vutura_common/event_loop.hpp"

#include "unifly_node.hpp"


namespace unifly {
	std::string username;
	std::string password;
	std::string client_id;
	std::string secret;
} // namespace airmap

void handle_periodic_timer(EventSource* es) {
	UniflyNode *node = static_cast<UniflyNode*>(es->_target_object);
	uint64_t num_timer_events;
	ssize_t recv_size = read(es->_fd, &num_timer_events, 8);
	(void) recv_size;

	node->periodic();
}

// main
int main(int argc, char* argv[])
{
	UniflyNode node(0);

	EventLoop eventloop;

	/*
	ListenerReplier utmsp(&node, SOCK_REQREP_UTMSP_COMMAND, handle_utmsp_update);
	eventloop.add(utmsp);

	Subscription gps_position(&node, SOCK_PUBSUB_GPS_POSITION, handle_position_update);
	eventloop.add(gps_position);

	Subscription uav_hb(&node, SOCK_PUBSUB_UAV_STATUS, handle_uav_hb);
	eventloop.add(uav_hb);

	*/
	Timer periodic_timer(&node, 2000, handle_periodic_timer);
	eventloop.add(periodic_timer);

	eventloop.add(node.gps_position_sub);
	eventloop.add(node.command_listener);

	node.start();
	eventloop.start();

	return 0;
}
