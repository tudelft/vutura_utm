#pragma once

#include "communicator.hpp"
#include "udp_sender.hpp"
#include "encryptor.hpp"
#include "buffer.hpp"

#include "vutura_common/publisher.hpp"

class AirmapNode {
public:
	enum AirmapState {
		STATE_INIT,
		STATE_LOGGED_IN,
		STATE_FLIGHT_REQUESTED,
		STATE_FLIGHT_AUTHORIZED,
		STATE_FLIGHT_STARTED
	};

	AirmapNode();

	void update_state(AirmapState new_state);
	int start();
	int request_flight();
	int periodic();
	int start_flight();
	int end_flight();
	int get_brief();
	int set_position(float latitude, float longitude, float alt_msl, float alt_agl);
	std::uint64_t getTimeStamp();

	bool _autostart_flight;

private:

	AirmapState _state;

	bool has_position_data() { return _has_position_data; }

	Communicator _communicator;
	UdpSender _udp;
	Encryptor _crypto;
	std::string _cipher;
	std::vector<std::uint8_t> _iv;
	Buffer _payload;

	Publisher _pub_utm_status_update;
	Publisher _pub_uav_command;

	AirmapTraffic _traffic;

	const uint8_t _encryptionType;
	std::string _pilotID;
	std::string _flightplanID;
	std::string _flightID;
	std::string _commsKey;
	uint32_t _comms_counter;

	bool _has_position_data;
	double _lat;
	double _lon;
	double _alt_msl;
	double _alt_agl;
};

