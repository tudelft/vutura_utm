#pragma once
#include <string>
#include <iostream>
#include <curl/curl.h>

#include "vutura_common/vutura_common.pb.h"
#include "vutura_common/helper.hpp"
#include "vutura_common/communicator.hpp"
#include "nng_event_loop/event_loop.hpp"
#include "nng_event_loop/timer.hpp"
#include "nng_event_loop/subscriber.hpp"
#include "nng_event_loop/replier.hpp"
#include "nng_event_loop/publisher.hpp"

#include "unifly_config.hpp"

class UniflyNode : public EventLoop {
public:

	enum UniflyState {
		STATE_INIT,
		STATE_LOGGED_IN,
		STATE_FLIGHT_REQUESTED,
		STATE_FLIGHT_AUTHORIZED,
		STATE_FLIGHT_RESCINDED,
		STATE_FLIGHT_CANCELLED,
		STATE_FLIGHT_STARTED,
		STATE_ARMED
	};

	UniflyNode(int instance, UniflyConfig *config);
	int init();
	void handle_gps_position(std::string message);
	void handle_command(std::string request, std::string &reply);
	int login();
	int request_flight();
	int get_user_id();
	int get_validation_results();
	int get_action_items();
	int request_permission(std::string uuid);
	int get_traffic_channels();
	int send_tracking_position();
	int send_takeoff();
	int send_land();
	int send_cancel_permission();
	int set_position(float latitude, float longitude, float alt_msl, float alt_agl);
	int periodic();
	uint64_t get_timestamp();

private:

	bool has_position_data() { return _has_position_data; }
	void update_state(UniflyState new_state);
	int start_flight();
	int end_flight();
	int end_active_flights();
	std::string state_name(UniflyState state);

	int _instance;
	UniflyState _state;
	UniflyConfig* _config;
	Communicator _comm;
	std::string _access_token;
	std::string _operation_unique_identifier;
	std::string _uas_uuid;
	std::string _user_uuid;
	std::string _permission_uuid;

	Timer _periodic_timer;
	Subscriber _gps_position_sub;
	Replier _command_replier;
	Publisher _pub_utm_status_update;

	bool _has_position_data;
	bool _streaming;
	double _lat;
	double _lon;
	double _alt_msl;
	double _alt_agl;
	double _pilot_lat;
	double _pilot_lon;
};
