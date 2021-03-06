#include <iostream>
#include <fstream>
#include <unistd.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <vutura_common/config.hpp>
#include "unifly_node.hpp"

using namespace std::placeholders;

UniflyNode::UniflyNode(int instance, UniflyConfig *config) :
	_instance(instance),
	_state(STATE_INIT),
	_config(config),
	_periodic_timer(this),
	_gps_position_sub(this),
	_command_replier(this),
	_pub_utm_status_update(socket_name(SOCK_PUBSUB_UTM_STATUS_UPDATE, _instance)),
	_lat(0.0),
	_lon(0.0),
	_alt_msl(0.0),
	_alt_agl(0.0),
	_has_position_data(false),
	_streaming(false)
{
	// hard-coded for now, can also be requested through the api
//	_uas_uuid = "e6755d9f-1b83-4993-a121-aaeab695996a"; // vutura iris
	_uas_uuid = "2b5e3966-3eec-4758-92a4-af236c4f7978"; // vutura disco PH-4HM
	_user_uuid = "e01c6f69-6107-456b-9031-6adc68afd24b"; // vutura
//	_uas_uuid = "897f53b7-61f0-4bd8-b1c4-9bc9a98d64b4"; // parrot disco in podium
	//	_user_uuid = "67f8413e-10a4-48bf-9948-c3d2d532cdee"; // podium
}

int UniflyNode::init()
{
	login();

	_periodic_timer.set_timeout_callback(std::bind(&UniflyNode::periodic, this));
	_periodic_timer.start_periodic(500);

	_gps_position_sub.set_receive_callback(std::bind(&UniflyNode::handle_gps_position, this, _1));
	_gps_position_sub.subscribe(socket_name(SOCK_PUBSUB_GPS_POSITION, _instance));

	_command_replier.set_receive_callback(std::bind(&UniflyNode::handle_command, this, _1, _2));
	_command_replier.listen(socket_name(SOCK_REQREP_UTMSP_COMMAND, _instance));
}

void UniflyNode::handle_gps_position(std::string message)
{
	// unpack msg
	GPSMessage gps_msg;
	gps_msg.ParseFromString(message);

	if (gps_msg.has_lat() && gps_msg.has_lon()) {
		set_position(gps_msg.lat() * 1e-7, gps_msg.lon() * 1e-7, gps_msg.alt_msl() * 1e-3, gps_msg.alt_agl() * 1e-3);
	}
}

void UniflyNode::handle_command(std::string request, std::string &reply)
{
	if (request == "land") {
		reply = "LANDING";
		send_land();

	} else if (request == "takeoff") {
		reply = "taking off";
		send_takeoff();

	} else if (request == "request_flight") {
		request_flight();
		reply = _operation_unique_identifier;

	} else if (request == "start_flight") {
		start_flight();
		reply = "starting";

	} else if (request == "end_flight") {
		end_flight();
		reply = "ending";

	} else if (request == "cancel_flight") {
		reply = "cancelling";
		send_cancel_permission();

	} else if (request == "action_items") {
		reply = "getting action items";
		get_action_items();

	} else if (request == "get_traffic_channels") {
		reply = "getting channels";
		get_traffic_channels();
	}
}

int UniflyNode::login()
{
	std::string url = "https://" + _config->host() + "/oauth/token";
	_comm.clear_headers();
	_comm.add_header("content-type", "application/x-www-form-urlencoded");
	_comm.add_header("authorization", "Basic " + _config->secret());
	_comm.add_header("accept", "application/json");

	std::string request_body = "username=" + _config->username() + "&password=" + _config->password() + "&grant_type=password";

	std::string res;
	_comm.post(url.c_str(), request_body.c_str(), res);

	auto j = nlohmann::json::parse(res);
	try {
		_access_token = j["access_token"].get<std::string>();
		std::cout << "Got access token: ";
		std::cout << _access_token << std::endl;
	} catch (...) {
		std::cerr << "Error getting access token" << std::endl;
		return -1;
	}

	// save access token
	std::string file = "/home/bart/unifly/token.txt";
	std::ofstream os;
	os.open(file, std::ios::trunc | std::ios::out);
	os << _access_token;

	os.close();


//	_takeoff = true;
//	_operation_unique_identifier = "7dfaf9e8-6c46-4b14-8ac4-a30d78b31e1b";
//	send_land();

//	request_flight();
//	get_validation_results();
//	get_action_items();

	end_active_flights();


	//get_permission(_permission_uuid);
	update_state(STATE_LOGGED_IN);

	return 0;
}

int UniflyNode::request_flight()
{
	if (_state != STATE_LOGGED_IN) {
		std::cerr << "Need to be in logged in state" << std::endl;
		return -1;
	}

	time_t current_time;
	time(&current_time);
	char now[21];
	strftime(now, 21, "%FT%TZ", gmtime(&current_time));

	time_t land_time = current_time + 10 * 60;
	char end[21];
	strftime(end, 21, "%FT%TZ", gmtime(&land_time));


	nlohmann::json request;

	// parse geofence file
	std::string geometry_file = "geo_hoi.json";
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
//	request["properties"]["environment"] = "RURAL";
	request["properties"]["purpose"] = "ROC";
	request["properties"]["rulesetCode"] = "ROC";
	request["properties"]["lineOfSightType"] = "B-VLOS";
	request["properties"]["pilotMobile"] = "0";
	request["properties"]["pilotUuid"] = _user_uuid;
	request["properties"]["crew"]["pilot"] = _user_uuid;
	request["properties"]["crew"]["contact"]["mobile"] = "0";
	request["properties"]["crew"]["user"] = _user_uuid;
	request["properties"]["uas"] = _uas_uuid;
	request["properties"]["takeOffPosition"] = nlohmann::json({});
	request["properties"]["landPosition"] = nlohmann::json({});



//	std::cout << request.dump(4) << std::endl;

	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/vnd.geo+json");

	std::string url = "https://" + _config->host() +  "/api/uasoperations";


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

	// Check if permission is needed
	get_validation_results();
	get_action_items();

	// request the permission if needed
	if (_permission_uuid == "") {
		update_state(STATE_FLIGHT_AUTHORIZED);
	} else {
		request_permission(_permission_uuid);
		update_state(STATE_FLIGHT_REQUESTED);
	}

	return 0;
}

int UniflyNode::get_user_id()
{
	std::string url = "https://" + _config->host() +  "/api/operator/users/me";

	std::string res;
	_comm.get(url.c_str(), res);

	try {
		nlohmann::json result = nlohmann::json::parse(res);
//		std::cout << result.dump(4) << std::endl;
		std::cout << "User UUID: " << result["uniqueIdentifier"] << std::endl;

	} catch (...) {
		std::cout << "Unusable result: " << res << std::endl;
	}


}

int UniflyNode::get_validation_results()
{
	while (true) {
		_comm.clear_headers();
		_comm.add_header("authorization", "Bearer " + _access_token);
		_comm.add_header("content-type", "application/json");

		if (_operation_unique_identifier == "") {
			std::cerr << "Unique Identifier for operation not set" << std::endl;
		}

		std::string url = "https://" + _config->host() +  "/api/uasoperations/" + _operation_unique_identifier + "/validationresults";


		std::string res;
		_comm.get(url.c_str(), res);

		nlohmann::json result;
		try {
			result = nlohmann::json::parse(res);
//			std::cout << result.dump(4) << std::endl;

		} catch (...) {
		}

		if (result.dump() != "[]") {
			std::cout << "validation complete" << std::endl;
			break;
		} else {
			std::cout << "validation pending..." << std::endl;
			sleep(1);
		}
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

	std::string url = "https://" + _config->host() +  "/api/uasoperations/" + _operation_unique_identifier + "/actionItems";


	std::string res;
	_comm.get(url.c_str(), res);

	nlohmann::json result;
	try {
		result = nlohmann::json::parse(res);

	} catch (...) {
	}

	//std::cout << result.dump(4) << std::endl;

	// request permission for action items
	std::string action_item_status;

	for (nlohmann::json::iterator it = result.begin(); it != result.end(); ++it) {
	  //std::cout << *it << '\n';
	  _permission_uuid = (*it)["uniqueIdentifier"].get<std::string>();
	  action_item_status = (*it)["status"].get<std::string>();
	  //std::cout << "Action item permission UUID: " << _permission_uuid << std::endl;
	  std::cout << "Permission status: " << action_item_status << std::endl;
//	  std::cout << (*it).dump(4) << std::endl;
	}

	if (action_item_status == "APPROVED") {
		update_state(STATE_FLIGHT_AUTHORIZED);
	} else if (action_item_status == "REJECTED") {
		update_state(STATE_LOGGED_IN);
	}

}

int UniflyNode::request_permission(std::string uuid)
{

	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "multipart/form-data");

	std::string url = "https://" + _config->host() +  "/api/uasoperations/" + _operation_unique_identifier + "/permissions/" + uuid + "/request";

	time_t current_time;
	time(&current_time);
	char now[21];
	strftime(now, 21, "%FT%TZ", gmtime(&current_time));

	nlohmann::json permission;
	permission["uniqueIdentifier"] = uuid;
	permission["permissionRemark"]["message"]["timestamp"] = now;


	std::string postfields = permission.dump();

	//std::cout << "Permission Request: " << permission.dump(4) << std::endl;

	std::string res;
	_comm.multipart_post(url.c_str(), postfields.c_str(), res);

	try {
		nlohmann::json result = nlohmann::json::parse(res);
		std::cout << "Permission request result: " << std::endl;
		std::cout << result.dump(4) << std::endl;
	} catch (...) {
		std::cout << "Could not parse response" << std::endl;
		return -1;
	}
}

int UniflyNode::get_traffic_channels()
{
	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/json");

	if (!_has_position_data) {
		std::cerr << "No position data available" << std::endl;
		return -1;
	}

	std::string url = "https://" + _config->host() +  "/api/realtime/position";


	nlohmann::json position;
	position["latitude"] = _lat;
	position["longitude"] = _lon;

//	nlohmann::json request;
//	request["bounds"]["location"] = position;
//	request["bounds"]["width"] = 50000;
//	request["bounds"]["height"] = 50000;

	std::string postfields = position.dump();


	std::string res;
	_comm.post(url.c_str(), postfields.c_str(), res);

	try {
		nlohmann::json result = nlohmann::json::parse(res);
		std::cout << "Traffic channels: " << std::endl;
		std::cout << result.dump(4) << std::endl;
	} catch (...) {
		std::cout << "Could not parse response" << std::endl;
	}
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

	std::string url = "https://" + _config->host() +  "/api/uasoperations/" + _operation_unique_identifier + "/uases/" + _uas_uuid + "/track";
	//std::cout << "URL: " << url << std::endl;

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

	std::string url = "https://" + _config->host() +  "/api/uasoperations/" + _operation_unique_identifier + "/uases/" + _uas_uuid + "/takeoff";

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
		std::cout << res << std::endl;
		return -1;
	}

	_streaming = true;
	return 0;
}

int UniflyNode::send_land()
{
	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/json");

//	_operation_unique_identifier = "";
//	_takeoff = true;
	if (_operation_unique_identifier == "") {
		std::cerr << "Unique Identifier for operation not set" << std::endl;
		return -1;
	}

	if (!_streaming) {
		std::cerr << "Not yet taken off" << std::endl;
		return -1;
	}

	time_t current_time;
	time(&current_time);
	char now[21];
	strftime(now, 21, "%FT%TZ", gmtime(&current_time));

	std::string url = "https://" + _config->host() +  "/api/uasoperations/" + _operation_unique_identifier + "/uases/" + _uas_uuid + "/landing";

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
		return -1;
	}

	_streaming = false;
	return 0;

}

int UniflyNode::send_cancel_permission()
{
	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/json");

	if (_permission_uuid == "") {
		std::cerr << "No permission to cancel" << std::endl;
		return -1;
	}
	std::string url = "https://" + _config->host() +  "/api/uasoperations/" + _operation_unique_identifier + "/permissions/" + _permission_uuid + "/cancellation";

	std::string postfields = "{}";

	std::string res;
	_comm.post(url.c_str(), postfields.c_str(), res);

	try {
		nlohmann::json result = nlohmann::json::parse(res);
//		std::cout << result.dump(4) << std::endl;
		std::cout << "Cancell permission, new status: " << result["status"] << std::endl;
	} catch (...) {
		std::cout << "Could not parse response" << std::endl;
		return -1;
	}

	update_state(STATE_FLIGHT_CANCELLED);

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
	if (_state == STATE_FLIGHT_REQUESTED) {
		get_action_items();
	}

	std::string state = state_name(_state);
	_pub_utm_status_update.publish(state);

	// send position?
	if (_operation_unique_identifier != "" && _has_position_data && _streaming && _state >= STATE_FLIGHT_RESCINDED) {
		send_tracking_position();
	} else {
//		std::cout << "Not sending, no position/takeoff?" << std::endl;
	}
}

uint64_t UniflyNode::get_timestamp()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return static_cast<std::uint64_t>(tp.tv_sec) * 1000L + tp.tv_usec / 1000;
}

void UniflyNode::update_state(UniflyNode::UniflyState new_state)
{
	if (new_state == STATE_LOGGED_IN) {
		// clear data
		_permission_uuid = "";
		_operation_unique_identifier = "";
	}
	_state = new_state;

	std::string state = state_name(_state);
	_pub_utm_status_update.publish(state);
}

int UniflyNode::start_flight()
{
	if (_state >= STATE_FLIGHT_RESCINDED) {
		std::cerr << "Already taken off" << std::endl;
		return -1;
	}
	if (send_takeoff() == 0) {
		update_state(STATE_FLIGHT_STARTED);
	}
}

int UniflyNode::end_flight()
{
	if (_state >= STATE_FLIGHT_RESCINDED) {
		if (send_land() == 0) {
			update_state(STATE_LOGGED_IN);
		}
	}
}

int UniflyNode::end_active_flights()
{
	_comm.clear_headers();
	_comm.add_header("authorization", "Bearer " + _access_token);
	_comm.add_header("content-type", "application/json");

	std::string url = "https://" + _config->host() +  "/api/uasoperations/?$filter=supersededBy%20eq%20%22null%22%20and%20(uas.status%20eq%20%22IN_FLIGHT%22)&$orderby=uas.status,geoZone.endTime,geoZone.startTime%20desc&$count=1&$skip=0";

	std::string res;
	_comm.get(url.c_str(), res);

	nlohmann::json result;
	try {
		result = nlohmann::json::parse(res);

	} catch (...) {
	}

	//std::cout << result.dump(4) << std::endl;

	try {
		for (nlohmann::json::iterator it = result.begin(); it != result.end(); ++it) {
//		  std::cout << *it << '\n';
//		  std::cout << "UUID: " << action_item_status << std::endl;
//		  std::cout << "NEXT: " << (*it).dump(4) << std::endl;
		  _operation_unique_identifier = (*it)["uniqueIdentifier"].get<std::string>();
		  _streaming = true;
		  std::cout << "Ending flight: " << _operation_unique_identifier << std::endl;
		  send_land();
		  _operation_unique_identifier = "";
		  _streaming = false;
		}

	} catch (...) {

	}
}

std::string UniflyNode::state_name(UniflyNode::UniflyState state)
{
	std::string state_name = "";
	switch (state) {
	case STATE_INIT:
		state_name = "init";
		break;

	case STATE_LOGGED_IN:
		state_name = "logged in";
		break;

	case STATE_FLIGHT_REQUESTED:
		state_name = "flight requested";
		break;

	case STATE_FLIGHT_AUTHORIZED:
		state_name = "flight authorized";
		break;

	case STATE_FLIGHT_STARTED:
		// serial number
		state_name = "flight started";
		break;

	case STATE_FLIGHT_RESCINDED:
		state_name = "flight rescinded";
		break;

	case STATE_FLIGHT_CANCELLED:
		state_name = "flight cancelled";
		break;

	case STATE_ARMED:
		state_name = "armed";
		break;

	default:
		state_name = "unknown state";
		break;
	}

	return state_name;
}
