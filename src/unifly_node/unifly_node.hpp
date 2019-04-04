#pragma once
#include <string>
#include <curl/curl.h>

#include "vutura_common/vutura_common.pb.h"
#include "vutura_common/communicator.hpp"
#include <vutura_common/subscription.hpp>

class UniflyNode {
public:
	UniflyNode();
	int start();
	int request_flight();
	int set_position(float latitude, float longitude, float alt_msl, float alt_agl);


	Subscription gps_position_sub;//(&node, SOCK_PUBSUB_GPS_POSITION, handle_position_update);

	static void position_update_callback(EventSource* es);

private:

	bool has_position_data() { return _has_position_data; }

	Communicator _comm;
	std::string _access_token;

	bool _has_position_data;
	double _lat;
	double _lon;
	double _alt_msl;
	double _alt_agl;
};
