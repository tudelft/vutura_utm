#include "vutura_common/event_loop.hpp"
#include "vutura_common/timer.hpp"

#include "vutura_common.pb.h"

#include "sim_node.hpp"


int main(int argc, char **argv)
{
	int instance = 0;
	if (argc > 1) {
		instance = atoi(argv[1]);
	}
	std::cout << "Instance " << std::to_string(instance) << std::endl;

	SimNode node(instance);
	EventLoop event_loop;

	Timer heartbeat_timer(&node, 500, node.heartbeat_timer_callback);
	event_loop.add(heartbeat_timer);

	event_loop.start();
}
