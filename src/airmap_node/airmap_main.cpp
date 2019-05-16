#include <string>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <poll.h>

// udp
#include <netdb.h>
// libcurl -- https://curl.haxx.se/libcurl/
#include <curl/curl.h>
// protobuf -- https://github.com/google/protobuf
#include "airmap_telemetry.pb.h"
// json -- https://github.com/nlohmann/json
#include <nlohmann/json.hpp>

#include "airmap_config.h"
#include "vutura_common/cxxopts.hpp"
#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "vutura_common.pb.h"
#include "airmap_traffic.hpp"

#include "airmap_node.hpp"

namespace airmap {
	std::string username;
	std::string password;
	std::string api_key;
	std::string client_id;
	std::string device_id;
} // namespace airmap

int parse_config(std::string config_file)
{
	nlohmann::json config;
	try {
		std::ifstream i(config_file);
		i >> config;
		airmap::username  = config["username"];
		airmap::password  = config["password"];
		airmap::api_key   = config["api_key"];
		airmap::client_id = config["client_id"];
		airmap::device_id = config["device_id"];

	} catch (...) {
		std::cerr << "Failed to parse config file " << config_file << std::endl;
		return -1;
	}
	std::cout << config.dump(4) << std::endl;
	return 0;
}

int main(int argc, char* argv[])
{
	int instance = 0;
	std::string config_file = "";
	std::string geometry_file = "";

	try
	{
		cxxopts::Options options(argv[0], " - Airmap Vutura node");
		options
				.positional_help("[optional args]")
				.show_positional_help();

		options
				.add_options()
				("i,instance", "Instance number, used for running multiple instances", cxxopts::value<int>(instance), "N")
				("c,config", "Airmap configuration file", cxxopts::value<std::string>(config_file), "FILE")
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
			if (parse_config(config_file.c_str()) != 0) {
				exit(EXIT_FAILURE);
			}
		} else {
			airmap::username = AIRMAP_USERNAME;
			airmap::password = AIRMAP_PASSWORD;
			airmap::api_key = AIRMAP_API_KEY;
			airmap::client_id = AIRMAP_CLIENT_ID;
			airmap::device_id = AIRMAP_DEVICE_ID;
		}

	} catch (const cxxopts::OptionException& e)
	{
		std::cout << "error parsing options: " << e.what() << std::endl;
		exit(1);
	}

	std::cout << "Instance " << std::to_string(instance) << std::endl;

	AirmapNode node(instance);

	// parse geofence file
	if (geometry_file != "") {
		try {
			nlohmann::json j;
			std::ifstream i(geometry_file);
			i >> j;

			// find the geofence
			for(auto it = j["features"].begin(); it != j["features"].end(); ++it) {
				if ((*it)["properties"]["name"] == "geofence") {
					node.set_geometry((*it)["geometry"]);
				}
			}

		} catch (...) {
			std::cerr << "Error parsing geofence file" << std::endl;
		}
	}

	node.init();
	node.run();

	return 0;
}
