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
#include "vutura_common.pb.h"

#include "airmap_node.hpp"

using namespace std::placeholders;

AirmapNode::AirmapNode(int instance) :
	_instance(instance),
	_autostart_flight(false),
	_state(STATE_INIT),
	_armed(false),
	_communicator(airmap::api_key),
	_udp(AIRMAP_TELEM_HOST, AIRMAP_TELEM_PORT),
	_periodic_timer(this),
	_utm_sp(this),
	_gps_position(this),
	_uav_hb(this),
	_pub_utm_status_update(socket_name(SOCK_PUBSUB_UTM_STATUS_UPDATE, instance)),
	_pub_uav_command(socket_name(SOCK_PUBSUB_UAV_COMMAND, instance)),
	_traffic(instance),
	_encryptionType(1),
	_comms_counter(1),
	_iv(16, 0),
	_has_position_data(false),
	_lat(0),
	_lon(0),
	_alt_msl(0),
	_alt_agl(0),
	_geometry(nullptr)
{

}

void AirmapNode::init()
{
	login();

	_periodic_timer.set_timeout_callback(std::bind(&AirmapNode::periodic, this));
	_periodic_timer.start_periodic(2000);

	_utm_sp.set_receive_callback(std::bind(&AirmapNode::handle_utm_sp, this, _1, _2));
	_utm_sp.listen(socket_name(SOCK_REQREP_UTMSP_COMMAND, _instance));

	_gps_position.set_receive_callback(std::bind(&AirmapNode::handle_position_update, this, _1));
	_gps_position.subscribe(socket_name(SOCK_PUBSUB_GPS_POSITION, _instance));

	_uav_hb.set_receive_callback(std::bind(&AirmapNode::handle_uav_hb, this, _1));
	_uav_hb.subscribe(socket_name(SOCK_PUBSUB_UAV_STATUS, _instance));
}

void AirmapNode::handle_utm_sp(std::string request, std::string &reply)
{

	_autostart_flight = false;
	if (request == "request_flight") {
		request_flight();

	} else if (request == "request_autoflight") {
		_autostart_flight = true;
		request_flight();
		start_flight();

	} else if (request == "abort_autoflight") {
		_autostart_flight = false;
		std::string command = "abort";
		_pub_uav_command.publish(command);

	} else if (request == "start_flight") {
		start_flight();

	} else if (request == "autostart_flight") {
		_autostart_flight = true;
		start_flight();

	} else if (request == "end_flight") {
		end_flight();

	} else if (request == "get_brief") {
		get_brief();

	} else {
		std::cout << "Received wrong command: " << request << std::endl;
		reply = "NOTOK";
		return;

	}

	reply = state_name(_state);
}

void AirmapNode::handle_position_update(std::string message)
{
	// unpack msg
	GPSMessage gps_msg;
	gps_msg.ParseFromString(message);

	if (gps_msg.has_lat() && gps_msg.has_lon()) {
		set_position(gps_msg.lat() * 1e-7, gps_msg.lon() * 1e-7, gps_msg.alt_msl() * 1e-3, gps_msg.alt_agl() * 1e-3);
		//std::cout << gps_msg.lat() * 1e-7 << ", " << gps_msg.lon() * 1e-7 << std::endl;
	}
}

void AirmapNode::handle_uav_hb(std::string message)
{
	// unpack msg
	UavHeartbeat hb;
	hb.ParseFromString(message);

	set_armed(hb.armed());

	if(hb.has_aborted()) {
		if (hb.aborted()) {
			abort();
		}
	}
}

void AirmapNode::update_state(AirmapNode::AirmapState new_state) {
	_state = new_state;

	std::string state = state_name(_state);
	_pub_utm_status_update.publish(state);
}

int AirmapNode::login() {
	if (-1 == _communicator.authenticate()) {
		std::cout << "Authentication Failed!" << std::endl;
		exit(EXIT_FAILURE);
		return -1;
	}

	if (-1 == _communicator.get_pilot_id(_pilotID)) {
		std::cout << "Get Pilot ID Failed!" << std::endl;
		return -1;
	}

	_communicator.end_all_active_flights(_pilotID);

	update_state(STATE_LOGGED_IN);

	return 0;
}

int AirmapNode::request_flight() {
	if (_state != STATE_LOGGED_IN) {
		std::cerr << "State not ready for request flight" << std::endl;
		return -1;
	}

	if (!has_position_data()) {
		std::cerr << "No position data available" << std::endl;
		return -1;
	}

	if (-1 == _communicator.create_flightplan(_lat, _lon, _geometry, _pilotID, _flightplanID)) {
		std::cout << "Flight Creation Failed!" << std::endl;
		return -1;
	}

	std::cout << "FlightplanID: " << _flightplanID << std::endl;

	_communicator.get_flight_briefing(_flightplanID);

	if (-1 == _communicator.submit_flight(_flightplanID, _flightID)) {
		std::cout << "Flight Submission Failed!" << std::endl;
		return -1;
	}

	std::cout << "FlightID: " << _flightID << std::endl;

	if (_communicator.flight_authorized()) {
		update_state(STATE_FLIGHT_AUTHORIZED);

	} else {
		update_state(STATE_FLIGHT_REQUESTED);

	}
}

void AirmapNode::periodic() {
	std::string state = state_name(_state);
	_pub_utm_status_update.publish(state);

	if (_state == STATE_FLIGHT_REQUESTED) {
		get_brief();

		if (_state == STATE_FLIGHT_AUTHORIZED && _autostart_flight) {
			std::cout << "Flight authorized, starting!" << std::endl;
			start_flight();
		}
	}
}

uint64_t AirmapNode::getTimeStamp() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return static_cast<std::uint64_t>(tp.tv_sec) * 1000L + tp.tv_usec / 1000;
}

int AirmapNode::set_armed(bool new_armed_state)
{
	std::string hb = "HB";
	_pub_uav_command.publish(hb);

	if (_state == STATE_FLIGHT_STARTED && new_armed_state) {
		update_state(STATE_ARMED);
	}

	if (_state == STATE_ARMED && !new_armed_state) {
		update_state(STATE_FLIGHT_STARTED);
	}

	if (new_armed_state != _armed) {

		if (new_armed_state) {
			// arm event
			std::cout << "Arm event" << std::endl;

		} else {
			// disarm event
			std::cout << "Disarm event" << std::endl;
			if (_state >= STATE_FLIGHT_STARTED) {
				std::cout << "Automatically ending flight" << std::endl;
				end_flight();
			}
		}

	}

	_armed = new_armed_state;
}

int AirmapNode::set_geometry(nlohmann::json geometry)
{
	_geometry = geometry;
}

void AirmapNode::abort()
{
	// we noticed that the requested flight was successfully aborted
	// depending on the state, take action
	switch (_state) {
	case STATE_INIT:
	case STATE_LOGGED_IN:
		// no action required
		break;

		break;
	case STATE_FLIGHT_REQUESTED:
	case STATE_FLIGHT_AUTHORIZED:
		_communicator.end_all_active_flights(_pilotID);
		update_state(STATE_LOGGED_IN);
		break;

	case STATE_FLIGHT_STARTED:
		end_flight();
		break;

	case STATE_ARMED:
		// don't do anything yet, will end flight when disarmed and in flight started state
		break;

	}

	std::string reply = "abort confirmed";
	_pub_uav_command.publish(reply);
}

std::string AirmapNode::state_name(AirmapNode::AirmapState state)
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
		_comms_counter = 1;
		state_name = "flight started";
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

int AirmapNode::get_brief() {
	_communicator.get_flight_briefing(_flightplanID);
	if (_communicator.flight_authorized()) {
		update_state(STATE_FLIGHT_AUTHORIZED);
	}
}

int AirmapNode::start_flight() {
	if (_state != STATE_FLIGHT_AUTHORIZED) {
		std::cerr << "Flight not authorized (yet)" << std::endl;
		return -1;
	}

	if (_autostart_flight) {
		// Send a command to arm the drone into mission mode
		std::string command = "start mission";
		// publish on socket
		_pub_uav_command.publish(command);
	}

	if (-1 == _communicator.start(_flightID, _commsKey)) {
		std::cout << "Communication Failed!" << std::endl;
		return -1;
	}

	std::cout << "telemetry comms key: " << _commsKey << std::endl;

	if (_traffic.start(_flightID, _communicator.get_token()) == -1) {
		std::cerr << "MQTT traffic could not start" << std::endl;
		_communicator.end(_flightID);
		return -1;
	}

	if (-1 == _udp.connect()) {
		std::cout << "Failed to connect!" << std::endl;
		_communicator.end(_flightID);
		_traffic.stop();
		return -1;
	}

	update_state(STATE_FLIGHT_STARTED);
}

int AirmapNode::end_flight() {
	if (_state != STATE_FLIGHT_STARTED) {
		std::cerr << "Flight not started, end first" << std::endl;
		return -1;
	}
	// end communication and flight
	_traffic.stop();
	_communicator.end(_flightID);
	_communicator.end_flight(_flightID);
	_commsKey = "";
	_flightID = "";
	_flightplanID = "";
	update_state(STATE_LOGGED_IN);
	std::cout << "Flight ended" << std::endl;
}

int AirmapNode::set_position(float latitude, float longitude, float alt_msl, float alt_agl) {
	_lat = latitude;
	_lon = longitude;
	_alt_msl = alt_msl;
	_alt_agl = alt_agl;
	_has_position_data = true;

	// If we have a comms key, send telemetry
	if (_commsKey.length() == 0) {
		return 0;
	}

	enum class Type : std::uint8_t { position = 1, speed = 2, attitude = 3, barometer = 4 };

	_payload.clear();
	// position
	airmap::telemetry::Position position;
	position.set_timestamp(getTimeStamp());
	position.set_latitude(_lat);
	position.set_longitude(_lon);
	position.set_altitude_agl(_alt_agl);
	position.set_altitude_msl(_alt_msl);
	position.set_horizontal_accuracy(10);
	auto messagePosition = position.SerializeAsString();
	_payload.add<std::uint16_t>(htons(static_cast<std::uint16_t>(Type::position)))
			.add<std::uint16_t>(htons(messagePosition.size()))
			.add(messagePosition);

	// encrypt payload
	_crypto.encrypt(_commsKey, _payload, _cipher, _iv);

	// build UDP packet
	Buffer packet;
	packet.add<std::uint32_t>(htonl(_comms_counter++))
			.add<std::uint8_t>(_flightID.size())
			.add(_flightID)
			.add<std::uint8_t>(_encryptionType)
			.add(_iv)
			.add(_cipher);
	std::string data = packet.get();
	int length = static_cast<int>(data.size());

	// send packet
	if (_udp.sendmsg(data.c_str(), length) == length) {
		//std::cout << "Telemetry packet sent successfully!" << std::endl;
	}

}
