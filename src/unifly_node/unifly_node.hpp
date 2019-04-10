#pragma once
#include <string>
#include <curl/curl.h>

#include "vutura_common/vutura_common.pb.h"
#include "vutura_common/helper.hpp"
#include "vutura_common/communicator.hpp"
#include <vutura_common/subscription.hpp>
#include <vutura_common/listener_replier.hpp>

class UniflyNode {
public:
	UniflyNode(int instance);
	int start();
	int request_flight();
	int get_user_id();
	int get_validation_results();
	int get_action_items();
	int get_permission(std::string uuid);
	int send_tracking_position();
	int send_takeoff();
	int send_land();
	int set_position(float latitude, float longitude, float alt_msl, float alt_agl);
	int periodic();
	uint64_t get_timestamp();


	Subscription gps_position_sub;//(&node, SOCK_PUBSUB_GPS_POSITION, handle_position_update);
	ListenerReplier command_listener;

	static void position_update_callback(EventSource* es);
	static void command_callback(EventSource* es);

private:

	bool has_position_data() { return _has_position_data; }

	Communicator _comm;
	std::string _access_token;
	std::string _operation_unique_identifier;
	std::string _uas_uuid;
	std::string _user_uuid;

	bool _has_position_data;
	bool _takeoff;
	double _lat;
	double _lon;
	double _alt_msl;
	double _alt_agl;
	double _pilot_lat;
	double _pilot_lon;
};
