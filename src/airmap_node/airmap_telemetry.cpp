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
// crypto -- https://www.cryptopp.com/
#include <cryptopp/aes.h>
#include <cryptopp/base64.h>
#include <cryptopp/ccm.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>

#include "airmap_config.h"
#include "vutura_common/config.hpp"
#include "vutura_common/vutura_common.pb.h"
#include "vutura_common/listener_replier.hpp"
#include "vutura_common/subscription.hpp"
#include "vutura_common/timer.hpp"
#include "vutura_common/publisher.hpp"
#include "vutura_common/event_loop.hpp"
#include "airmap_traffic.hpp"

// command to build with g++
// g++ -std=c++1y -o "telemetry" telemetry.pb.cc telemetry.cpp -lcurl -lcryptopp -lprotobuf

// simple sawtooth wave simulation
#define BUMP(val, dt, mx, initval) val += dt; if (val > mx) val = initval; return val;

class TelemetrySource {
public:
	virtual std::uint64_t getTimeStamp();
	virtual double getLattitude();
	virtual double getLongtitude();
	virtual float getAgl();
	virtual float getMsl();
	virtual float getHorizAccuracy();

private:

};

class Simulator : public TelemetrySource
{
public:
	Simulator() {
		init();
	}
	void init() {
		m_lat = 52.170365387094016;
		m_lon = 4.4171905517578125;
		m_agl = 10.0;
		m_msl = 10.0;
		m_horizAccuracy = 0.0;
		m_yaw = 0.0;
		m_pitch = 0.0;
		m_roll = 0.0;
		m_velocity_x = 10.0;
		m_velocity_y = 10.0;
		m_velocity_z = 10.0;
		m_pressure = 1012.0;
	}

	std::uint64_t getTimeStamp() {
		struct timeval tp;
		gettimeofday(&tp, NULL);
		return static_cast<std::uint64_t>(tp.tv_sec) * 1000L + tp.tv_usec / 1000;
	}

	double getLattitude() { BUMP(m_lat, 0.00000, 90, 52.170365387094016) }
	double getLongtitude() { BUMP(m_lon, 0.00002, 90, 4.4171905517578125) }
	float getAgl() { BUMP(m_agl, 0.0, 100.0, 10.0) }
	float getMsl() { BUMP(m_msl, 0.0, 100.0, 10.0) }
	float getHorizAccuracy() { BUMP(m_horizAccuracy, 0.0, 10.0, 10.0) }
	float getYaw() { BUMP(m_yaw, 1.0, 360.0, 0.0) }
	float getPitch() { BUMP(m_pitch, 1.0, 10.0, -10.0) }
	float getRoll() { BUMP(m_roll, 1.0, 10.0, -10.0) }
	float getVelocityX() { BUMP(m_velocity_x, 0.0, 100.0, 10.0) }
	float getVelocityY() { BUMP(m_velocity_y, 0.0, 100.0, 10.0) }
	float getVelocityZ() { BUMP(m_velocity_z, 0.0, 100.0, 10.0) }
	float getPressure() { BUMP(m_pressure, 0.0, 1013.0, 1012.0) }

private:
	// position
	double m_lat;
	double m_lon;
	float m_agl;
	float m_msl;
	float m_horizAccuracy;
	// attitude
	float m_yaw;
	float m_pitch;
	float m_roll;
	// speed
	float m_velocity_x;
	float m_velocity_y;
	float m_velocity_z;
	// baro
	float m_pressure;
};

// Communicator
class Communicator
{
public:
	Communicator(const std::string& apiKey) {
		m_url = "https://" AIRMAP_HOST;
		m_headers = NULL;
		m_headers = curl_slist_append(m_headers, ("X-API-KEY: " + apiKey).c_str());
		m_authorization_status = "";
		curl_global_init(CURL_GLOBAL_DEFAULT);
	}

	int authenticate() {
		std::string url = "https://" AIRMAP_SSO_HOST "/oauth/ro";
		std::string res;
		char post_data[] = "grant_type=password"
				   "&client_id=" AIRMAP_CLIENT_ID
				"&connection=Username-Password-Authentication"
				"&username=" AIRMAP_USERNAME
				"&password=" AIRMAP_PASSWORD
				"&scope=openid offline_access"
				"&device=" AIRMAP_DEVICE_ID;
		if (CURLE_OK != curl_post(url.c_str(), m_headers, post_data, res))
			return -1;
		// parse result to get token
		std::cout << res << std::endl;
		auto j = nlohmann::json::parse(res);
		try {
			m_token = j["id_token"];
			m_headers = curl_slist_append(m_headers, ("Authorization: Bearer " + m_token).c_str());
			m_headers = curl_slist_append(m_headers, "Accept: application/json");
			m_headers = curl_slist_append(m_headers, "Content-Type: application/json");
			m_headers = curl_slist_append(m_headers, "charsets: utf-8");
		}
		catch (...) {
			return -1; // failed authentication!
		}
		return 0;
	}

	int get_pilot_id(std::string& pilotID) {
		std::string url = m_url + "/pilot/v2/profile";
		std::string res;
		if (CURLE_OK != curl_get(url.c_str(), m_headers, res)) {
			return -1;
		}
		std::cout << "request pilot id:" << std::endl << res << std::endl;
		auto j = nlohmann::json::parse(res);
		try {
			pilotID = j["data"]["id"];
		}
		catch (...) {
			return -1;
		}
	}

	int create_flightplan(const float latitude, const float longitude, const std::string& pilotID, std::string& flightplanID) {
		std::string url = m_url + "/flight/v2/plan";

		time_t current_time;
		time(&current_time);
		char now[21];
		strftime(now, 21, "%FT%TZ", gmtime(&current_time));

		time_t land_time = current_time + 30 * 60;
		char end[21];
		strftime(end, 21, "%FT%TZ", gmtime(&land_time));

		nlohmann::json request = {
			{"pilot_id", pilotID},
			{"start_time", now},
			{"end_time", end},
			{"buffer", 20},
			{"max_altitude_agl", 120},
			{"takeoff_latitude", latitude},
			{"takeoff_longitude", longitude},
			{"rulesets", {"custom_z5d341_drone_rules"}},
			{"geometry", {
				 {"type", "Point"},
				 {"coordinates", {
					  longitude, latitude
				  }}
			 }}
		};

		std::cout << request.dump(4) << std::endl;

		std::string res;
		if (CURLE_OK != curl_post(url.c_str(), m_headers, request.dump().c_str(), res)) {
			return -1;
		}
		// parse result to get flightID
		std::cout << res << std::endl;
		auto j = nlohmann::json::parse(res);
		try {
			flightplanID = j["data"]["id"];
		}
		catch (...) {
			return -1; // failed flight creation!
		}
		return 0;
	}

	int get_flight_briefing(const std::string& flightplanID) {
		std::string url = m_url + "/flight/v2/plan/" + flightplanID + "/briefing";
		std::string res;
		if (CURLE_OK != curl_get(url.c_str(), m_headers, res)) {
			return -1;
		}

		try{
			auto j = nlohmann::json::parse(res);
			std::cout << "Briefing result:" << std::endl << j.dump(4) << std::endl;
			if (j["data"]["authorizations"].size() == 0) {
				m_authorization_status = "not required";
			} else {
//				std::cout << "Authorization request: " << j["data"]["authorizations"][0]["status"] << std::endl;
				m_authorization_status = j["data"]["authorizations"][0]["status"];
			}
			std::cout << "Authorization status: " << m_authorization_status << std::endl;
		}
		catch (...) {
			std::cout << "Error get briefing" << std::endl;
			return -1;
		}
	}

	bool flight_authorized() {
		bool ret = false;
		if (m_authorization_status == "authorized" ||
		    m_authorization_status == "not required") {
			ret = true;
		}
		return ret;
	}

	int submit_flight(const std::string& flightplanID, std::string& flightID) {
		std::string url = m_url + "/flight/v2/plan/" + flightplanID + "/submit";
		std::string res;
		if (CURLE_OK != curl_post(url.c_str(), m_headers, "", res)) {
			return -1;
		}
		std::cout << "submit result: " << res << std::endl;

		auto j = nlohmann::json::parse(res);
		try {
			flightID = j["data"]["flight_id"];
		}
		catch (...) {
			return -1;
		}
		return 0;
	}

	int start(std::string flightID, std::string& key) {
		std::string url = m_url + "/flight/v2/" + flightID + "/start-comm";
		std::string res;
		if (CURLE_OK != curl_post(url.c_str(), m_headers, "", res))
			return -1;
		// parse result to get key
		auto j = nlohmann::json::parse(res);
		try {
			key = j["data"]["key"];
		}
		catch (...) {
			return -1; // failed communication!
		}
		return 0;
	}

	void end(std::string flightID) {
		std::string url = m_url + "/flight/v2/" + flightID + "/end-comm";
		std::string res;
		curl_post(url.c_str(), m_headers, "", res);
	}

	void end_flight(std::string flightID) {
		std::string url = m_url + "/flight/v2/" + flightID + "/end";
		std::string res;
		curl_post(url.c_str(), m_headers, "", res);
	}

	void query_telemetry(std::string flightID) {
		std::string url = m_url + "/archive/v1/telemetry/position?flight_id=" + flightID;
		std::string res;
		curl_get(url.c_str(), m_headers, res);
		std::cout << res << std::endl;
	}

	std::string& get_token() {
		return m_token;
	}

	int end_all_active_flights(std::string pilotID) {
		// must have a pilot id for this

		time_t current_time;
		time(&current_time);
		char now[21];
		strftime(now, 21, "%FT%TZ", gmtime(&current_time));

		std::string url = m_url + "/flight/v2/?pilot_id=" + pilotID + "&end_after=" + now;
		std::cout << "URL: \"" << url << "\"" << std::endl;

		std::string res;
		curl_get(url.c_str(), m_headers, res);

		// parse the results with json interpreter
		std::cout << res << std::endl;
		auto j = nlohmann::json::parse(res);
		try {
			auto active_flights = j["data"]["results"];
			for (int i = 0; i < active_flights.size(); i++) {
				std::cout << "Active flight_id: " << active_flights[i]["id"] << std::endl;
				end_flight(active_flights[i]["id"]);
				std::cout << "Ended" << std::endl;
			}
		}
		catch (...) {
			return -1;
		}

		return 0;
	}

private:
	static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
		data->append((char*)ptr, size * nmemb);
		return size * nmemb;
	}

	unsigned int curl_post(const char* url, const struct curl_slist* headers, const char* postfields, std::string& response) {
		CURLcode res = CURLE_FAILED_INIT;
		CURL *curl = curl_easy_init();

		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Communicator::writeFunction);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

			res = curl_easy_perform(curl);
			if (res != CURLE_OK) {
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			}

			curl_easy_cleanup(curl);
		}

		return res;
	}

	unsigned int curl_get(const char* url, const struct curl_slist* headers, std::string& response) {
		CURLcode res = CURLE_FAILED_INIT;
		CURL *curl = curl_easy_init();

		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Communicator::writeFunction);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

			res = curl_easy_perform(curl);
			if (res != CURLE_OK) {
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			}

			curl_easy_cleanup(curl);
		}

		return res;
	}

	struct curl_slist *m_headers;
	std::string m_url;
	std::string m_token;
	std::string m_authorization_status;
};

// UdpSender
class UdpSender
{
public:
	UdpSender(const char* host, const int port) : m_host(host), m_port(port), m_sockfd(-1) {}
	int connect() {
		// create socket
		m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_sockfd < 0)
			return -1;

		// get server
		struct hostent *server = gethostbyname(m_host);
		if (server == NULL)
			return -1;

		// set port and IP protocol
		bzero(&m_serveraddr, sizeof(m_serveraddr));
		m_serveraddr.sin_family = AF_INET;
		bcopy(server->h_addr, &m_serveraddr.sin_addr.s_addr, server->h_length);
		m_serveraddr.sin_port = htons(m_port);

		return 0;
	}

	int sendmsg(const char* msg, int msglen) {
		if (m_sockfd < 0)
			return -1;
		return sendto(m_sockfd, msg, msglen, 0, (struct sockaddr*)&m_serveraddr, sizeof(m_serveraddr));
	}

private:
	const char* m_host;
	int  m_port;
	int m_sockfd;
	struct sockaddr_in m_serveraddr;
};

// Buffer
class Buffer
{
public:
	void clear() {
		m_buffer.clear();
	}

	template <typename T>
	Buffer& add(const T& value) {
		return add(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	Buffer& add(const std::string& s) {
		return add(s.c_str(), s.size());
	}

	Buffer& add(const std::vector<std::uint8_t>& v) {
		return add(reinterpret_cast<const char*>(v.data()), v.size());
	}

	Buffer& add(const char* value, std::size_t size) {
		m_buffer.insert(m_buffer.end(), value, value + size);
		return *this;
	}

	const std::string& get() const {
		return m_buffer;
	}

private:
	std::string m_buffer;
};

// Encryptor
class Encryptor
{
public:
	Encryptor() {}
	void encrypt(const std::string& key, const Buffer& payload, std::string& cipher, std::vector<std::uint8_t>& iv) {
		cipher.clear();
		// generate iv
		using byte = unsigned char;
		CryptoPP::AutoSeededRandomPool rnd;
		rnd.GenerateBlock(iv.data(), iv.size());

		// decode key
		std::string decoded_key;
		CryptoPP::StringSource decoder(key, true, new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded_key)));

		// encrypt payload to cipher
		CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption enc;
		enc.SetKeyWithIV(reinterpret_cast<const byte*>(decoded_key.c_str()), decoded_key.size(), iv.data());
		CryptoPP::StringSource s(payload.get(), true, new CryptoPP::StreamTransformationFilter(enc, new CryptoPP::StringSink(cipher)));
	}
};

// Payload
class Payload {
public:
	Payload(TelemetrySource *telemetry_source) : m_source(telemetry_source) {}
	void build(Buffer& payload) {
		payload.clear();
		// message types
		enum class Type : std::uint8_t { position = 1, speed = 2, attitude = 3, barometer = 4 };

		// position
		airmap::telemetry::Position position;
		position.set_timestamp(m_source->getTimeStamp());
		position.set_latitude(m_source->getLattitude());
		position.set_longitude(m_source->getLongtitude());
		position.set_altitude_agl(m_source->getAgl());
		position.set_altitude_msl(m_source->getMsl());
		position.set_horizontal_accuracy(m_source->getHorizAccuracy());
		auto messagePosition = position.SerializeAsString();
		payload.add<std::uint16_t>(htons(static_cast<std::uint16_t>(Type::position)))
				.add<std::uint16_t>(htons(messagePosition.size()))
				.add(messagePosition);

		/*
		// speed
		airmap::telemetry::Speed speed;
		speed.set_timestamp(m_source->getTimeStamp());
		speed.set_velocity_x(m_source->getVelocityX());
		speed.set_velocity_y(m_source->getVelocityY());
		speed.set_velocity_z(m_source->getVelocityZ());
		auto messageSpeed = speed.SerializeAsString();
		payload.add<std::uint16_t>(htons(static_cast<std::uint16_t>(Type::speed)))
				.add<std::uint16_t>(htons(messageSpeed.size()))
				.add(messageSpeed);

		// attitude
		airmap::telemetry::Attitude attitude;
		attitude.set_timestamp(m_source->getTimeStamp());
		attitude.set_yaw(m_source->getYaw());
		attitude.set_pitch(m_source->getPitch());
		attitude.set_roll(m_source->getRoll());
		auto messageAttitude = attitude.SerializeAsString();
		payload.add<std::uint16_t>(htons(static_cast<std::uint16_t>(Type::attitude)))
				.add<std::uint16_t>(htons(messageAttitude.size()))
				.add(messageAttitude);

		// barometer
		airmap::telemetry::Barometer barometer;
		barometer.set_timestamp(m_source->getTimeStamp());
		barometer.set_pressure(m_source->getPressure());
		auto messageBarometer = barometer.SerializeAsString();
		payload.add<std::uint16_t>(htons(static_cast<std::uint16_t>(Type::barometer)))
				.add<std::uint16_t>(htons(messageBarometer.size()))
				.add(messageBarometer);
		*/
	}

private:
	TelemetrySource *m_source;
};

class AirmapNode {
public:
	enum AirmapState {
		STATE_INIT,
		STATE_LOGGED_IN,
		STATE_FLIGHT_REQUESTED,
		STATE_FLIGHT_AUTHORIZED,
		STATE_FLIGHT_STARTED
	};

	AirmapNode() :
		_autostart_flight(false),
		_state(STATE_INIT),
		_communicator(AIRMAP_API_KEY),
		_udp(AIRMAP_TELEM_HOST, AIRMAP_TELEM_PORT),
		_encryptionType(1),
		_comms_counter(1),
		_iv(16, 0),
		_has_position_data(false),
		_lat(0),
		_lon(0),
		_alt_msl(0),
		_alt_agl(0)
	{
		// open socket for publishing utm status update
		int rv;

		if ((rv = nng_pub0_open(&_pub_utm_status_update)) != 0) {
			std::cerr << "nng_pub0_open pub traffic: " << nng_strerror(rv) << std::endl;
		}

		if ((rv = nng_listen(_pub_utm_status_update, SOCK_PUBSUB_UTM_STATUS_UPDATE, NULL, 0)) != 0) {
			std::cerr << "nng_listen pub traffic: " << nng_strerror(rv) << std::endl;
		}

		if ((rv = nng_pub0_open(&_pub_uav_command)) != 0) {
			std::cerr << "nng_pub0_open uav command: " << nng_strerror(rv) << std::endl;
		}

		if ((rv = nng_listen(_pub_uav_command, SOCK_PUBSUB_UAV_COMMAND, NULL, 0)) != 0) {
			std::cerr << "nng_listen uav command: " << nng_strerror(rv) << std::endl;
		}

	}

	void update_state(AirmapState new_state) {
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
			state = "flight started";
			break;

		default:
			state = "unknown state";
			break;
		}

		// publish on socket
		nng_msg *msg;
		nng_msg_alloc(&msg, state.length());
		memcpy((char*)nng_msg_body(msg), state.c_str(), state.length());
		nng_sendmsg(_pub_utm_status_update, msg, 0);
	}

	int start() {
		if (-1 == _communicator.authenticate()) {
			std::cout << "Authentication Failed!" << std::endl;
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

	int request_flight() {
		if (!has_position_data()) {
			return -1;
		}

		if (-1 == _communicator.create_flightplan(_lat, _lon, _pilotID, _flightplanID)) {
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

	int periodic() {
		if (_state == STATE_FLIGHT_REQUESTED) {
			get_brief();

			if (_state == STATE_FLIGHT_AUTHORIZED) {
				std::cout << "Flight authorized, starting!" << std::endl;
				start_flight();
			}
		}
	}

	int start_flight() {
		if (_flightID.size() == 0) {
			std::cout << "_flightID empty" << std::endl;
			return -1;
		}

		if (!_communicator.flight_authorized()) {
			std::cout << "Authorization status does not (yet) allow start of flight" << std::endl;
			update_state(STATE_FLIGHT_REQUESTED);
			return -1;
		}

		if (_autostart_flight) {
			// Send a command to arm the drone into mission mode
			std::string command = "start mission";
			// publish on socket
			nng_msg *msg;
			nng_msg_alloc(&msg, command.length());
			memcpy((char*)nng_msg_body(msg), command.c_str(), command.length());
			nng_sendmsg(_pub_utm_status_update, msg, 0);
		}

		if (-1 == _communicator.start(_flightID, _commsKey)) {
			std::cout << "Communication Failed!" << std::endl;
			return -1;
		}

		_traffic.start(_flightID, _communicator.get_token());

		std::cout << "telemetry comms key: " << _commsKey << std::endl;

		if (-1 == _udp.connect()) {
			std::cout << "Failed to connect!" << std::endl;
			return -1;
		}

		// serial number
		_comms_counter = 1;

		update_state(STATE_FLIGHT_STARTED);
	}

	int end_flight() {
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

	int get_brief() {
		_communicator.get_flight_briefing(_flightplanID);
	}

	int set_position(float latitude, float longitude, float alt_msl, float alt_agl) {
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

	std::uint64_t getTimeStamp() {
		struct timeval tp;
		gettimeofday(&tp, NULL);
		return static_cast<std::uint64_t>(tp.tv_sec) * 1000L + tp.tv_usec / 1000;
	}

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

	nng_socket _pub_utm_status_update;
	nng_socket _pub_uav_command;

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

void handle_utmsp_update(EventSource* es)
{
	ListenerReplier *rep = static_cast<ListenerReplier*>(es);
	AirmapNode *node = static_cast<AirmapNode*>(rep->_target_object);
	std::string request = rep->get_message();

	node->_autostart_flight = false;
	if (request == "request_flight") {
		node->request_flight();

	} else if (request == "start_flight") {
		node->start_flight();

	} else if (request == "autostart_flight") {
		node->_autostart_flight = true;
		node->start_flight();

	} else if (request == "end_flight") {
		node->end_flight();

	} else if (request == "get_brief") {
		node->get_brief();

	}
	const std::string reply = "OK";
	rep->send_response(reply);
}

void handle_request(AirmapNode* node, nng_msg *msg)
{
	std::string request((char*)nng_msg_body(msg), (char*)nng_msg_body(msg) + nng_msg_len(msg));
	if (request == "request_flight") {
		node->request_flight();

	} else if (request == "start_flight") {
		node->start_flight();

	} else if (request == "end_flight") {
		node->end_flight();
	}
}

void handle_position_update(EventSource* es)
{
	AirmapNode *node = static_cast<AirmapNode*>(es->_target_object);
	Subscription *sub = static_cast<Subscription*>(es);

	std::string msg = sub->get_message();

	// unpack msg
	GPSMessage gps_msg;
	gps_msg.ParseFromString(msg);

	if (gps_msg.has_lat() && gps_msg.has_lon()) {
		node->set_position(gps_msg.lat() * 1e-7, gps_msg.lon() * 1e-7, gps_msg.alt_msl() * 1e-3, gps_msg.alt_agl() * 1e-3);
		//std::cout << gps_msg.lat() * 1e-7 << ", " << gps_msg.lon() * 1e-7 << std::endl;
	}
}

void handle_periodic_timer(EventSource* es) {
	AirmapNode *node = static_cast<AirmapNode*>(es->_target_object);
	uint64_t num_timer_events;
	ssize_t recv_size = read(es->_fd, &num_timer_events, 8);
	(void) recv_size;

	node->periodic();
}

// main
int main(int argc, char* argv[])
{
	AirmapNode node;
	// authenticate etc
	node.start();
//	node.set_position(52.170365387094016, 4.4171905517578125, 10, 10);

	EventLoop eventloop;

	ListenerReplier utmsp(&node, SOCK_REQREP_UTMSP_COMMAND, handle_utmsp_update);
	eventloop.add(utmsp);

	Subscription gps_position(&node, SOCK_PUBSUB_GPS_POSITION, handle_position_update);
	eventloop.add(gps_position);

	Timer periodic_timer(&node, 2000, handle_periodic_timer);
	eventloop.add(periodic_timer);

	eventloop.start();

	return 0;
}
