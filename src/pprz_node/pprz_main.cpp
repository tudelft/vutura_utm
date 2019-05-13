#include <iostream>
#include "pprz_node.hpp"

#include "vutura_common/event_loop.hpp"

// main
int main(int argc, char* argv[])
{
	int instance = 0;
	std::cout << "Instance " << std::to_string(instance) << std::endl;

	PaparazziNode node(instance);

	EventLoop loop;
	loop.add(node.gpos_source);

	loop.start();
}
