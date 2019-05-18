#ifndef AIRMAP_NODE_HPP
#define AIRMAP_NODE_HPP

#include <nlohmann/json.hpp>
#include "airmap_communicator.hpp"
#include "udp_sender.hpp"
#include "encryptor.hpp"
#include "buffer.hpp"

#include "nng_event_loop/event_loop.hpp"
#include "nng_event_loop/timer.hpp"
#include "nng_event_loop/replier.hpp"
#include "nng_event_loop/subscriber.hpp"
#include "nng_event_loop/publisher.hpp"

#include "airmap_traffic.hpp"

class AirmapNode : public EventLoop {
public:
	enum AirmapState {
		STATE_INIT,
		STATE_LOGGED_IN,
		STATE_FLIGHT_REQUESTED,
		STATE_FLIGHT_AUTHORIZED,
		STATE_FLIGHT_STARTED,
		STATE_ARMED
	};

	AirmapNode(int instance);

	void init();
	void handle_utm_sp(std::string request, std::string &reply);
	void handle_position_update(std::string message);
	void handle_uav_hb(std::string message);
	void update_state(AirmapState new_state);
	int login();
	int request_flight();
	void periodic();
	int start_flight();
	int end_flight();
	int get_brief();
	int set_position(float latitude, float longitude, float alt_msl, float alt_agl);
	std::uint64_t getTimeStamp();
	int set_armed(bool new_armed_state);
	int set_geometry(nlohmann::json geofence);

	bool _autostart_flight;

private:
	void abort();
	bool has_position_data() { return _has_position_data; }
	std::string state_name(AirmapState state);

	int _instance;
	AirmapState _state;
	bool _armed;

	AirmapCommunicator _communicator;
	UdpSender _udp;
	Encryptor _crypto;
	std::string _cipher;
	std::vector<std::uint8_t> _iv;
	Buffer _payload;

	Timer _periodic_timer;
	Replier _utm_sp;
	Subscriber _gps_position;
	Subscriber _uav_hb;

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

	nlohmann::json _geometry;
};

#endif // AIRMAP_NODE_HPP
