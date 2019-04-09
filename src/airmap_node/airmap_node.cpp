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

AirmapNode::AirmapNode(int instance) :
	_autostart_flight(false),
	_state(STATE_INIT),
	_communicator(airmap::api_key),
	_udp(AIRMAP_TELEM_HOST, AIRMAP_TELEM_PORT),
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

void AirmapNode::update_state(AirmapNode::AirmapState new_state) {
	_state = new_state;

	std::string state = "";
	switch (new_state) {
	case STATE_INIT:
		state = "init";
		break;

	case STATE_LOGGED_IN:
		state = "logged in";
		break;

	case STATE_FLIGHT_REQUESTED:
		state = "flight requested";
		break;

	case STATE_FLIGHT_AUTHORIZED:
		state = "flight authorized";
		break;

	case STATE_FLIGHT_STARTED:
		// serial number
		_comms_counter = 1;
		state = "flight started";
		break;

	default:
		state = "unknown state";
		break;
	}

	_pub_utm_status_update.publish(state);
}

int AirmapNode::start() {
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

int AirmapNode::periodic() {
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

int AirmapNode::set_armed(bool armed)
{
	if (!armed) {
		// disarm event
		std::cout << "Disarm event" << std::endl;
		if (_state == STATE_FLIGHT_STARTED) {
			std::cout << "Automatically ending flight" << std::endl;
			end_flight();
		}
	}
}

int AirmapNode::set_geometry(nlohmann::json geometry)
{
	_geometry = geometry;
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
		return -1;
	}

	if (-1 == _udp.connect()) {
		std::cout << "Failed to connect!" << std::endl;
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
	_communicator.query_telemetry(_flightID);
	_communicator.end(_flightID);
	_communicator.end_flight(_flightID);
	_commsKey = "";
	_flightID = "";
	_flightplanID = "";
	update_state(STATE_LOGGED_IN);
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
