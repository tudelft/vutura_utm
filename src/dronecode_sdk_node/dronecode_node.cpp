#include <unistd.h>
#include <iostream>
#include <future>

#include "dronecode_node.h"

#include "vutura_common/helper.hpp"
#include "vutura_common/event_loop.hpp"
#include "vutura_common/udp_source.hpp"
#include "vutura_common/timer.hpp"
#include "vutura_common/subscription.hpp"
#include "vutura_common/listener_replier.hpp"

#include "vutura_common.pb.h"

namespace dronecode_node {
	DronecodeNode* instance;
}

DronecodeNode::DronecodeNode(int instance) :
        gps_pub(socket_name(SOCK_PUBSUB_GPS_POSITION, instance)),
        uav_armed_pub(socket_name(SOCK_PUBSUB_UAV_STATUS, instance)),
	_mission(nullptr),
	_uuid(0),
        _armed(false),
        _guided_mode(false),
        _avoiding(false)
{
	dronecode_node::instance = this;
	_dc.register_on_discover(dronecode_on_discover_callback);
	_dc.register_on_timeout(dronecode_on_timeout);
	_dc.add_any_connection("udp://");
}

void DronecodeNode::uav_command(std::string command)
{
	if (command == "start mission") {
		std::cout << "Starting mission" << std::endl;
		{
			std::cout << "Starting mission." << std::endl;
			auto prom = std::make_shared<std::promise<Mission::Result>>();
			auto future_result = prom->get_future();
			_mission->start_mission_async([prom](Mission::Result result) {
			    prom->set_value(result);
			    std::cout << "Started mission." << std::endl;
			});

			const Mission::Result result = future_result.get();
			std::cerr << Mission::result_str(result) << std::endl;
		}
	} else {
		std::cout << "Received command: " << command << std::endl;
	}
}

void DronecodeNode::dronecode_on_discover_callback(uint64_t uuid)
{
	dronecode_node::instance->_uuid = uuid;
	std::cout << "Discovered UUID: " << uuid << std::endl;
	if (dronecode_node::instance->_mission == nullptr) {
		dronecode_node::instance->_mission = new Mission(dronecode_node::instance->_dc.system(uuid));
	}
}

void DronecodeNode::dronecode_on_timeout(uint64_t uuid)
{
	dronecode_node::instance->_uuid = 0;
	std::cout << "Timeout, removing " << uuid << std::endl;
	if (dronecode_node::instance->_mission != nullptr) {
		delete dronecode_node::instance->_mission;
	}
}

void DronecodeNode::heartbeat_timer_callback(EventSource *es)
{
	// read the timer
	uint64_t num_timer_events;
	ssize_t recv_size = read(es->_fd, &num_timer_events, 8);
	(void) recv_size;

	Timer *tim = static_cast<Timer*>(es);
	DronecodeNode *node = static_cast<DronecodeNode*>(tim->_target_object);

	std::cout << "Periodic" << std::endl;
}

void DronecodeNode::uav_command_callback(EventSource *es)
{
	Subscription *rep = static_cast<Subscription*>(es);
	DronecodeNode *node = static_cast<DronecodeNode*>(rep->_target_object);
	std::string message = rep->get_message();

	node->uav_command(message);
}

void DronecodeNode::avoidance_command_callback(EventSource *es)
{
	ListenerReplier *listrep = static_cast<ListenerReplier*>(es);
	DronecodeNode *node = static_cast<DronecodeNode*>(listrep->_target_object);
	std::string message = listrep->get_message();
	node->uav_command(message);
	listrep->send_response("OK");
}

