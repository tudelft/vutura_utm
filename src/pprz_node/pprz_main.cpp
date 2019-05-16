#include <iostream>
#include "pprz_node.hpp"

#include "vutura_common/event_loop.hpp"
#include "vutura_common/subscription.hpp"

// main
int main(int argc, char* argv[])
{
	int instance = 0;
	std::cout << "Instance " << std::to_string(instance) << std::endl;

	PaparazziNode node(instance);

	EventLoop loop;
        loop.add(node.pprz_comm);

        Subscription avoidance_sub(&node, socket_name(SOCK_REQREP_AVOIDANCE_COMMAND, instance), node.avoidance_command_callback);
        loop.add(avoidance_sub);

	loop.start();
}
