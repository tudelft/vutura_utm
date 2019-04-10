#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <vutura_common/config.hpp>
#include "unifly_config.h"
#include "unifly_node.hpp"


UniflyNode::UniflyNode() :
	gps_position_sub(this, SOCK_PUBSUB_GPS_POSITION, position_update_callback),
	_lat(0.0),
	_lon(0.0),
	_alt_msl(0.0),
	_alt_agl(0.0),
	_has_position_data(false)
{

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

	request_flight();
	get_action_items();

	return 0;
}

int UniflyNode::request_flight()
{
	time_t current_time;
	time(&current_time);
	char now[21];
	strftime(now, 21, "%FT%TZ", gmtime(&current_time));

	time_t land_time = current_time + 30 * 60;
	char end[21];
	strftime(end, 21, "%FT%TZ", gmtime(&land_time));


	nlohmann::json request;

	// parse geofence file
	std::string geometry_file = "/home/bart/airmap/geo_hoi.json";
	nlohmann::json geometry;
	if (geometry_file != "") {
		try {
			nlohmann::json geometry;
			std::ifstream i(geometry_file);
			i >> geometry;
			//std::cout << geometry.dump(4) << std::endl;
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
	request["properties"]["crew"]["pilot"] = "e01c6f69-6107-456b-9031-6adc68afd24b";
	request["properties"]["crew"]["user"] = "e01c6f69-6107-456b-9031-6adc68afd24b";
//	request["properties"]["takeOffPosition"] = nlohmann::json({});
//	request["properties"]["landPosition"] = nlohmann::json({});



	//std::cout << request.dump(4) << std::endl;

	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/vnd.geo+json");

	std::string url = "https://" UNIFLY_HOST "/api/uasoperations";


	std::string postfields = request.dump();

	std::string res;
	_comm.post(url.c_str(), postfields.c_str(), res);

	nlohmann::json result = nlohmann::json::parse(res);
//	std::cout << result.dump(4) << std::endl;

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

int UniflyNode::get_action_items()
{
	if (_operation_unique_identifier == "") {
		std::cerr << "Unique Identifier for operation not set" << std::endl;
	}

	std::string url = "https://" UNIFLY_HOST "/api/uasoperations/" + _operation_unique_identifier + "/actionItems";


	std::string res;
	_comm.get(url.c_str(), res);

	nlohmann::json result = nlohmann::json::parse(res);

	std::cout << result.dump(4) << std::endl;
}

int UniflyNode::set_position(float latitude, float longitude, float alt_msl, float alt_agl)
{
	_lat = latitude;
	_lon = longitude;
	_alt_msl = alt_msl;
	_alt_agl = alt_agl;
	_has_position_data = true;
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
