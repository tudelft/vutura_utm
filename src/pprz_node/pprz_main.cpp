#include <iostream>

#include "vutura_common/cxxopts.hpp"
#include "pprz_node.hpp"

// main
int main(int argc, char* argv[])
{
	int instance = 0;

	// Parse options
	try
	{
		cxxopts::Options options(argv[0], " - Airmap Simulation Node");
		options
				.positional_help("[optional args]")
				.show_positional_help();

		options
				.add_options()
				("i,instance", "Instance number, used for running multiple instances", cxxopts::value<int>(instance), "N")
				("help", "Print help")
				;

		auto result = options.parse(argc, argv);

		if (result.count("help"))
		{
			std::cout << options.help() << std::endl;
			exit(0);
		}

	} catch (const cxxopts::OptionException& e)
	{
		std::cout << "error parsing options: " << e.what() << std::endl;
		exit(1);
	}

	std::cout << "Instance: " << std::to_string(instance) << std::endl;

	PaparazziNode node(instance);

	node.init();
	node.run();
}
