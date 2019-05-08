#include "vutura_common/event_loop.hpp"
#include "vutura_common/timer.hpp"
#include "vutura_common/subscription.hpp"
#include "vutura_common/listener_replier.hpp"

#include "dronecode_node.h"

int main(int argc, char **argv)
{
	int instance = 0;
	if (argc > 1) {
		instance = atoi(argv[1]);
	}
	std::cout << "Instance " << std::to_string(instance) << std::endl;

	EventLoop event_loop;

	DronecodeNode node(instance);

	Timer heartbeat_timer(&node, 1000, node.heartbeat_timer_callback);
	event_loop.add(heartbeat_timer);

	Subscription uav_command_sub(&node, socket_name(SOCK_PUBSUB_UAV_COMMAND, instance), node.uav_command_callback);
	event_loop.add(uav_command_sub);

	ListenerReplier avoidance_rep(&node, socket_name(SOCK_REQREP_AVOIDANCE_COMMAND, instance), node.avoidance_command_callback);
	event_loop.add(avoidance_rep);

	event_loop.start();
}
