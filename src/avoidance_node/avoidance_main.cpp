#include "vutura_common/config.hpp"
#include "vutura_common/event_loop.hpp"
#include "vutura_common/timer.hpp"
#include "vutura_common/subscription.hpp"
#include "vutura_common/cxxopts.hpp"
#include "avoidance_config.hpp"
#include "avoidance_geometry.hpp"

#include "avoidance_node.hpp"

int main(int argc, char* argv[])
{
	int instance = 0;
	std::string config_file = "";
	std::string geometry_file = "";

	Avoidance_config config;
	Avoidance_geometry geometry;

	try
	{
		cxxopts::Options options(argv[0], " - Airmap Avoidance node");
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
		
		
		if (result.count("c")) 
		{
			// parse config
			std::cout << "Using config file: " << config_file << std::endl;
			if (config.parse_config(config_file.c_str()) != 0) {
				exit(EXIT_FAILURE);
			}
		} else {
			std::cout << "No configuration file given, using standard configuration" << std::endl;
		}

		if (result.count("g"))
		{
			// parse geojson features
			std::cout << "Using geometry file: " << geometry_file << std::endl;
			if (geometry.parse_geometry(geometry_file.c_str()) != 0) {
				exit(EXIT_FAILURE);
			}
		} else {
			std::cout << "No geometry file selected, no flightplplan and geofence known to the avoidance computer" << std::endl;
		}
	} catch (const cxxopts::OptionException& e)
	{
		std::cout << "error parsing options: " << e.what() << std::endl;
		exit(1);
	}
    std::cout << "Instance " << std::to_string(instance) << std::endl;

	AvoidanceNode node(instance, config, geometry);
        node.InitialiseSSD();
        node.InitialiseLogger();
	EventLoop event_loop;

	Timer periodic_timer(&node, 200, node.periodic_timer_callback);
	event_loop.add(periodic_timer);

	Subscription traffic_sub(&node, socket_name(SOCK_PUBSUB_TRAFFIC_INFO, instance), node.traffic_callback);
	event_loop.add(traffic_sub);

	Subscription gps_sub(&node, socket_name(SOCK_PUBSUB_GPS_POSITION, instance), node.gps_position_callback);
	event_loop.add(gps_sub);

//	double x,y,dist;
//	node.get_relative_coordinates(52.17164192042854, 4.421846866607666, 52.174050125852816, 4.425022602081299, &x, &y);
//	dist = sqrt(x*x + y*y);
//	std::cout << "x: " << std::to_string(x) << "\ty: " << std::to_string(y) << "\tdist: " << std::to_string(dist) <<  std::endl;

	event_loop.start();
}
