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
#include "vutura_common/vutura_common.pb.h"
#include "vutura_common/listener_replier.hpp"
#include "vutura_common/subscription.hpp"
#include "vutura_common/timer.hpp"
#include "vutura_common/publisher.hpp"
#include "vutura_common/event_loop.hpp"
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

void handle_utmsp_update(EventSource* es)
{
	ListenerReplier *rep = static_cast<ListenerReplier*>(es);
	AirmapNode *node = static_cast<AirmapNode*>(rep->_target_object);
	std::string request = rep->get_message();

	node->_autostart_flight = false;
	if (request == "request_flight") {
		node->request_flight();

	} else if (request == "start_flight") {
		node->start_flight();

	} else if (request == "autostart_flight") {
		node->_autostart_flight = true;
		node->start_flight();

	} else if (request == "end_flight") {
		node->end_flight();

	} else if (request == "get_brief") {
		node->get_brief();

	}
	const std::string reply = "OK";
	rep->send_response(reply);
}

void handle_position_update(EventSource* es)
{
	AirmapNode *node = static_cast<AirmapNode*>(es->_target_object);
	Subscription *sub = static_cast<Subscription*>(es);

	std::string msg = sub->get_message();

	// unpack msg
	GPSMessage gps_msg;
	gps_msg.ParseFromString(msg);

	if (gps_msg.has_lat() && gps_msg.has_lon()) {
		node->set_position(gps_msg.lat() * 1e-7, gps_msg.lon() * 1e-7, gps_msg.alt_msl() * 1e-3, gps_msg.alt_agl() * 1e-3);
		//std::cout << gps_msg.lat() * 1e-7 << ", " << gps_msg.lon() * 1e-7 << std::endl;
	}
}

void handle_uav_hb(EventSource* es) {
	AirmapNode *node = static_cast<AirmapNode*>(es->_target_object);
	Subscription *sub = static_cast<Subscription*>(es);

	std::string msg = sub->get_message();

	// unpack msg
	UavHeartbeat hb;
	hb.ParseFromString(msg);

	node->set_armed(hb.armed());
}

void handle_periodic_timer(EventSource* es) {
	AirmapNode *node = static_cast<AirmapNode*>(es->_target_object);
	uint64_t num_timer_events;
	ssize_t recv_size = read(es->_fd, &num_timer_events, 8);
	(void) recv_size;

	node->periodic();
}

// main
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
//				std::cout << "Name: " << (*it)["properties"]["name"] << std::endl;
				if ((*it)["properties"]["name"] == "geofence") {
					node.set_geometry((*it)["geometry"]);
				}
			}

//			std::cout << j.dump(4) << std::endl;

//			node.set_geometry(j);
		} catch (...) {
			std::cerr << "Error parsing geofence file" << std::endl;
		}
	}


	// authenticate etc
	node.start();

	EventLoop eventloop;

	ListenerReplier utmsp(&node, socket_name(SOCK_REQREP_UTMSP_COMMAND, instance), handle_utmsp_update);
	eventloop.add(utmsp);

	Subscription gps_position(&node, socket_name(SOCK_PUBSUB_GPS_POSITION, instance), handle_position_update);
	eventloop.add(gps_position);

	Subscription uav_hb(&node, socket_name(SOCK_PUBSUB_UAV_STATUS, instance), handle_uav_hb);
	eventloop.add(uav_hb);

	Timer periodic_timer(&node, 2000, handle_periodic_timer);
	eventloop.add(periodic_timer);

	eventloop.start();

	return 0;
}
