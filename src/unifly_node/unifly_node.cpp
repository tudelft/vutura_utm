#include <iostream>
#include <fstream>
#include <unistd.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <vutura_common/config.hpp>
#include "unifly_config.h"
#include "unifly_node.hpp"


UniflyNode::UniflyNode(int instance) :
	gps_position_sub(this, socket_name(SOCK_PUBSUB_GPS_POSITION, instance), position_update_callback),
	command_listener(this, socket_name(SOCK_REQREP_UTMSP_COMMAND, instance), command_callback),
	_lat(0.0),
	_lon(0.0),
	_alt_msl(0.0),
	_alt_agl(0.0),
	_has_position_data(false),
	_takeoff(false)
{
	// hard-coded for now, can also be requested through the api
	_uas_uuid = "e6755d9f-1b83-4993-a121-aaeab695996a";
	_user_uuid = "e01c6f69-6107-456b-9031-6adc68afd24b";
}

int UniflyNode::start()
{
	std::string url = "https://" UNIFLY_SSO_HOST "/oauth/token";
	_comm.clear_headers();
	_comm.add_header("content-type", "application/x-www-form-urlencoded");
	_comm.add_header("authorization", "Basic " UNIFLY_SECRET);
	_comm.add_header("accept", "application/json");

	std::string request_body = "username=" UNIFLY_USERNAME "&password=" UNIFLY_PASSWORD "&grant_type=password";

	std::string res;
	_comm.post(url.c_str(), request_body.c_str(), res);

	auto j = nlohmann::json::parse(res);
	try {
		_access_token = j["access_token"].get<std::string>();
		std::cout << "Got access token:" << std::endl;
		std::cout << _access_token << std::endl;
	} catch (...) {
		std::cerr << "Error getting access token" << std::endl;
		return -1;
	}

//	get_user_id();
//	return 0;
	request_flight();
	sleep(2); // beun, validation
	get_validation_results();
	get_action_items();

	return 0;
}

int UniflyNode::request_flight()
{
	time_t current_time;
	time(&current_time);
	char now[21];
	strftime(now, 21, "%FT%TZ", gmtime(&current_time));

	time_t land_time = current_time + 10 * 60;
	char end[21];
	strftime(end, 21, "%FT%TZ", gmtime(&land_time));


	nlohmann::json request;

	// parse geofence file
	std::string geometry_file = "/home/bart/unifly/geo_delft.json";
	nlohmann::json geometry;
	if (geometry_file != "") {
		try {
			nlohmann::json geometry;
			std::ifstream i(geometry_file);
			i >> geometry;
//			std::cout << geometry.dump(4) << std::endl;
			request["geometry"] = geometry;
		} catch (...) {
			std::cerr << "Error parsing geofence file" << std::endl;
		}
	}

//	geometry["type"] = "Point";
//	geometry["coordinates"] = {4.4177055, 52.1692467};
	//request["geometry"] = geometry;

	request["type"] = "Feature";
	request["properties"]["startTime"] = now;
	request["properties"]["endTime"] = end;
	request["properties"]["lowerLimit"] = 0;
	request["properties"]["upperLimit"] = 120;
	request["properties"]["name"] = "TU Delft";
	request["properties"]["crew"]["pilot"] = _user_uuid;
	request["properties"]["crew"]["user"] = _user_uuid;
	request["properties"]["uas"] = _uas_uuid;
//	request["properties"]["takeOffPosition"] = nlohmann::json({});
//	request["properties"]["landPosition"] = nlohmann::json({});



	std::cout << request.dump(4) << std::endl;

	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/vnd.geo+json");

	std::string url = "https://" UNIFLY_HOST "/api/uasoperations";


	std::string postfields = request.dump();

	std::string res;
	_comm.post(url.c_str(), postfields.c_str(), res);

	nlohmann::json result;
	try {
		result = nlohmann::json::parse(res);
		std::cout << result.dump(4) << std::endl;

	} catch (...) {
	}

	try {
		_operation_unique_identifier = result["uniqueIdentifier"];
	} catch (...) {
		std::cerr << "Failed to extract operation unique identifier" << std::endl;
		return -1;
	}

	std::cout << "Operation UniqueIdentifier: " << _operation_unique_identifier << std::endl;


	if(!has_position_data()) {
		std::cerr << "No position data available" << std::endl;
		return -1;
	}
}

int UniflyNode::get_user_id()
{
	std::string url = "https://" UNIFLY_HOST "/api/operator/users/me";

	std::string res;
	_comm.get(url.c_str(), res);

	try {
		nlohmann::json result = nlohmann::json::parse(res);
		std::cout << result.dump(4) << std::endl;

	} catch (...) {
		std::cout << "Unusable result: " << res << std::endl;
	}


}

int UniflyNode::get_validation_results()
{
	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/json");

	if (_operation_unique_identifier == "") {
		std::cerr << "Unique Identifier for operation not set" << std::endl;
	}

	std::string url = "https://" UNIFLY_HOST "/api/uasoperations/" + _operation_unique_identifier + "/validationresults";


	std::string res;
	_comm.get(url.c_str(), res);

	nlohmann::json result;
	try {
		result = nlohmann::json::parse(res);
		std::cout << result.dump(4) << std::endl;

	} catch (...) {
	}

}

int UniflyNode::get_action_items()
{

	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/json");

	if (_operation_unique_identifier == "") {
		std::cerr << "Unique Identifier for operation not set" << std::endl;
	}

	std::string url = "https://" UNIFLY_HOST "/api/uasoperations/" + _operation_unique_identifier + "/actionItems";


	std::string res;
	_comm.get(url.c_str(), res);

	nlohmann::json result;
	try {
		result = nlohmann::json::parse(res);

	} catch (...) {
	}

	std::cout << result.dump(4) << std::endl;
}

int UniflyNode::send_tracking_position()
{

	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/json");

	if (_operation_unique_identifier == "") {
		std::cerr << "Unique Identifier for operation not set" << std::endl;
		return -1;
	}

	if (!_has_position_data) {
		std::cerr << "No position data available" << std::endl;
		return -1;
	}

	std::string url = "https://" UNIFLY_HOST "/api/uasoperations/" + _operation_unique_identifier + "/uases/" + _uas_uuid + "/track";
	std::cout << "URL: " << url << std::endl;

	time_t current_time;
	time(&current_time);
	char now[21];
	strftime(now, 21, "%FT%TZ", gmtime(&current_time));

	nlohmann::json tracking;
	// REQUIRED
	tracking["location"]["latitude"] = _lat;
	tracking["location"]["longitude"] = _lon;
	tracking["pilotLocation"]["latitude"] = _pilot_lat;
	tracking["pilotLocation"]["longitude"] = _pilot_lon;
	tracking["timestamp"] = now;
	// OPTIONAL
	tracking["altitudeAGL"] = _alt_agl;

	std::string postfields = tracking.dump();

	std::cout << "Tracking: " << tracking.dump(4) << std::endl;

	std::string res;
	_comm.post(url.c_str(), postfields.c_str(), res);

	std::cout << "Result within quotes: \"" << res << "\"" << std::endl;
	try {
		nlohmann::json result = nlohmann::json::parse(res);
		std::cout << result.dump(4) << std::endl;
	} catch (...) {
		std::cout << "Could not parse response" << std::endl;
	}

}

int UniflyNode::send_takeoff()
{
	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/json");

	if (_operation_unique_identifier == "") {
		std::cerr << "Unique Identifier for operation not set" << std::endl;
		return -1;
	}

	if (!_has_position_data) {
		std::cerr << "No position data available" << std::endl;
		return -1;
	}

	time_t current_time;
	time(&current_time);
	char now[21];
	strftime(now, 21, "%FT%TZ", gmtime(&current_time));

	std::string url = "https://" UNIFLY_HOST "/api/uasoperations/" + _operation_unique_identifier + "/uases/" + _uas_uuid + "/takeoff";

	nlohmann::json takeoff;
	takeoff["pilotLocation"]["latitude"] = _pilot_lat;
	takeoff["pilotLocation"]["longitude"] = _pilot_lon;
	takeoff["startTime"] = now;
	std::string postfields = takeoff.dump();

	std::cout << "Takeoff: " << takeoff.dump(4) << std::endl;

	std::string res;
	_comm.post(url.c_str(), postfields.c_str(), res);

	try {
		nlohmann::json result = nlohmann::json::parse(res);
		std::cout << result.dump(4) << std::endl;
	} catch (...) {
		std::cout << "Could not parse response" << std::endl;
	}

	_takeoff = true;
	return 0;
}

int UniflyNode::send_land()
{
	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/json");

	if (_operation_unique_identifier == "") {
		std::cerr << "Unique Identifier for operation not set" << std::endl;
		return -1;
	}

	if (!_takeoff) {
		std::cerr << "Not yet taken off" << std::endl;
		return -1;
	}

	time_t current_time;
	time(&current_time);
	char now[21];
	strftime(now, 21, "%FT%TZ", gmtime(&current_time));

	std::string url = "https://" UNIFLY_HOST "/api/uasoperations/" + _operation_unique_identifier + "/uases/" + _uas_uuid + "/landing";

	nlohmann::json takeoff;
	takeoff["pilotLocation"]["latitude"] = _pilot_lat;
	takeoff["pilotLocation"]["longitude"] = _pilot_lon;
	takeoff["endTime"] = now;
	std::string postfields = takeoff.dump();

	std::cout << "Landing: " << takeoff.dump(4) << std::endl;

	std::string res;
	_comm.post(url.c_str(), postfields.c_str(), res);

	std::cout << res << std::endl;
	try {
		nlohmann::json result = nlohmann::json::parse(res);
		std::cout << result.dump(4) << std::endl;
	} catch (...) {
		std::cout << "Could not parse response" << std::endl;
	}

	_takeoff = false;
	return 0;

}

int UniflyNode::set_position(float latitude, float longitude, float alt_msl, float alt_agl)
{
	_lat = latitude;
	_lon = longitude;
	_alt_msl = alt_msl;
	_alt_agl = alt_agl;
	if (_has_position_data == false) {
		_has_position_data = true;
		_pilot_lat = _lat;
		_pilot_lon = _lon;
	}

}

int UniflyNode::periodic()
{
	// send position?
	if (_operation_unique_identifier != "" && _has_position_data && _takeoff) {
		send_tracking_position();
	} else {
		std::cout << "Not sending, no position/takeoff?" << std::endl;
	}
}

uint64_t UniflyNode::get_timestamp()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return static_cast<std::uint64_t>(tp.tv_sec) * 1000L + tp.tv_usec / 1000;
}

void UniflyNode::position_update_callback(EventSource *es)
{
	std::cout << "Got mavlink position" << std::endl;

	UniflyNode *node = static_cast<UniflyNode*>(es->_target_object);
	Subscription *sub = static_cast<Subscription*>(es);

	std::string msg = sub->get_message();

	// unpack msg
	GPSMessage gps_msg;
	gps_msg.ParseFromString(msg);

	if (gps_msg.has_lat() && gps_msg.has_lon()) {
		node->set_position(gps_msg.lat() * 1e-7, gps_msg.lon() * 1e-7, gps_msg.alt_msl() * 1e-3, gps_msg.alt_agl() * 1e-3);
	}
}

void UniflyNode::command_callback(EventSource *es)
{
	ListenerReplier* rep = static_cast<ListenerReplier*>(es);
	UniflyNode* node = static_cast<UniflyNode*>(es->_target_object);

	std::string command = rep->get_message();

	std::string reply = "NOTOK";
	if (command == "land") {
		reply = "LANDING";
		node->send_land();

	} else if (command == "takeoff") {
		reply = "taking off";
		node->send_takeoff();
	}

	rep->send_response(reply);
}
