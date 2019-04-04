#include <string>
#include <cstdint>
#include <iostream>
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
	if (argc > 1) {
		instance = atoi(argv[1]);
	}
	std::cout << "Instance " << std::to_string(instance) << std::endl;

	AirmapNode node(instance);
	// authenticate etc
	node.start();
	//	node.set_position(52.170365387094016, 4.4171905517578125, 10, 10);

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
