#include <iostream>
#include "experimental_node.hpp"

// main
int main(int argc, char* argv[])
{
	int instance = 0;
	std::cout << "Instance " << std::to_string(instance) << std::endl;

	ExperimentalNode node(instance);

	node.init();
	node.run();

}
