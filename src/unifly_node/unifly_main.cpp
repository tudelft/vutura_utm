#include <unistd.h>

#include "vutura_common/cxxopts.hpp"
#include "vutura_common/config.hpp"
#include "vutura_common/vutura_common.pb.h"

#include "unifly_node.hpp"

int main(int argc, char* argv[])
{
	int instance = 0;
	std::string config_file = "";
	std::string geometry_file = "";

	try
	{
		cxxopts::Options options(argv[0], " - Unifly Vutura Node");
		options
				.positional_help("[optional args]")
				.show_positional_help();

		options
				.add_options()
				("i,instance", "Instance number, used for running multiple instances", cxxopts::value<int>(instance), "N")
				("c,config", "Unifly configuration file", cxxopts::value<std::string>(config_file), "FILE")
				("g,geometry", "GeoJSON Geometry file", cxxopts::value<std::string>(geometry_file), "FILE")
				("help", "Print help")
				;

		auto result = options.parse(argc, argv);

		if (result.count("help"))
		{
			std::cout << options.help() << std::endl;
			exit(0);
		}

		if (result.count("c")) {
			// parse config
			std::cout << "Using config file: " << config_file << std::endl;
		}

	} catch (const cxxopts::OptionException& e)
	{
		std::cout << "error parsing options: " << e.what() << std::endl;
		exit(1);
	}

	std::cout << "Instance " << std::to_string(instance) << std::endl;

	UniflyConfig config(config_file);
	UniflyNode node(instance, &config);

	node.init();
	node.run();

	return 0;
}
