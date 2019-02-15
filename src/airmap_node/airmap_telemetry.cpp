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
// nanomsg next gen
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/protocol/reqrep0/rep.h>

#include "airmap_config.h"

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
			std::string token = j["id_token"];
			m_headers = curl_slist_append(m_headers, ("Authorization: Bearer " + token).c_str());
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

		time_t land_time = current_time + 110;
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
			{"geometry", {
				 {"type", "Point"},
				 {"coordinates", {
					  latitude, longitude
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
	AirmapNode() :
		_communicator(AIRMAP_API_KEY),
		_encryptionType(1),

		_has_position_data(false),
		_lat(0),
		_lon(0)
	{
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

		std::string _flightID;
		if (-1 == _communicator.submit_flight(_flightplanID, _flightID)) {
			std::cout << "Flight Submission Failed!" << std::endl;
			return -1;
		}

		std::cout << "FlightID: " << _flightID << std::endl;
	}

	int start_flight() {

	}

	int end_flight() {

	}

	int set_position(float latitude, float longitude) {
		_lat = latitude;
		_lon = longitude;
		_has_position_data = true;
	}


private:

	bool has_position_data() { return _has_position_data; }

	Communicator _communicator;
	const uint8_t _encryptionType;
	std::string _pilotID;
	std::string _flightplanID;
	std::string _flightID;

	bool _has_position_data;
	double _lat;
	double _lon;
};

void handle_request(AirmapNode* node, nng_msg *msg)
{
	node->request_flight();
}

void fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

// main
int main(int argc, char* argv[])
{
	AirmapNode node;
	// authenticate etc
	node.start();

	const float latitude = 52.170365387094016;
	// longitude
	const float longitude = 4.4171905517578125;

	node.set_position(latitude, longitude);

	// Listen for nng messages in an event loop
	nng_socket utmsp_sock;
	int utmsp_fd = -1;
	int rv;

	// Configure reply topic for utmsp request
	if ((rv = nng_rep0_open(&utmsp_sock)) != 0) {
		fatal("nng_rep0_open", rv);
	}

	if ((rv = nng_listen(utmsp_sock, "ipc:///tmp/utmsp.sock", NULL, NNG_FLAG_NONBLOCK))) {
		fatal("nng_listen", rv);
	}

	if ((rv = nng_getopt_int(utmsp_sock, NNG_OPT_RECVFD, &utmsp_fd))) {
		fatal("nng_getopt", rv);
	}

	// Configure file descriptors for event listening
	const unsigned int num_fds = 1;
	struct pollfd fds[num_fds];
	fds[0].fd = utmsp_fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	bool should_exit = false;
	while (!should_exit) {
		int rv = poll(fds, num_fds, 3000);
		if (rv == 0) {
			std::cout << "Timeout..." << std::endl;
			continue;
		}

		if (fds[0].revents & POLLIN) {
			const char* reply = "Hallo!";
			nng_msg *msg;
			nng_recvmsg(utmsp_sock, &msg, 0);
			std::cout << (char*)nng_msg_body(msg) << " " << strlen(reply) << std::endl;
			handle_request(&node, msg);
			nng_msg_realloc(msg, strlen(reply));
			memcpy((char*)nng_msg_body(msg), reply, strlen(reply));
			nng_sendmsg(utmsp_sock, msg, 0);
		}
	}




	return 0;






	// API Key
	const std::string apiKey = AIRMAP_API_KEY;
	// latitude
//	const float latitude = 52.170365387094016;
	// longitude
//	const float longitude = 4.4171905517578125;
	// aes-256-cbc
	const uint8_t encryptionType = 1;

	// authenticate, create flight, and start communications
	Communicator communicator(apiKey);
	if (-1 == communicator.authenticate()) {
		std::cout << "Authentication Failed!" << std::endl;
		return -1;
	}

	std::string pilotID;
	if (-1 == communicator.get_pilot_id(pilotID)) {
		std::cout << "Get Pilot ID Failed!" << std::endl;
	}

	std::string flightplanID;
	if (-1 == communicator.create_flightplan(latitude, longitude, pilotID, flightplanID)) {
		std::cout << "Flight Creation Failed!" << std::endl;
		return -1;
	}

	std::cout << "FlightplanID: " << flightplanID << std::endl;

	std::string flightID;
	if (-1 == communicator.submit_flight(flightplanID, flightID)) {
		std::cout << "Flight Submission Failed!" << std::endl;
		return -1;
	}

	std::cout << "FlightID: " << flightID << std::endl;

	return 0;
	usleep(15 * 1000000);
	std::string key;
	if (-1 == communicator.start(flightID, key)) {
		std::cout << "Communication Failed!" << std::endl;
		return -1;
	}

	std::cout << "telemetry comms key: " << key << std::endl;

	// hostname
	const char* hostname = "telemetry.airmap.com";
	// port
	const int port = 16060;

	// connect to server
	UdpSender udp(hostname, port);
	if (-1 == udp.connect()) {
		std::cout << "Failed to connect!" << std::endl;
		return -1;
	}

	// serial number
	static std::uint32_t counter = 1;

	// prepare builder
	Simulator sim;
	Payload payloadBuilder(&sim);
	Buffer payload;

	// prepare encryptor
	Encryptor crypto;
	std::string cipher;
	std::vector<std::uint8_t> iv(16, 0);


	// send 100 packets @ 5 Hz
	for (int i = 0; i < 5*60; ++i) {
		// build payload
		payloadBuilder.build(payload);

		// encrypt payload
		crypto.encrypt(key, payload, cipher, iv);

		// build UDP packet
		Buffer packet;
		packet.add<std::uint32_t>(htonl(counter++))
				.add<std::uint8_t>(flightID.size())
				.add(flightID)
				.add<std::uint8_t>(encryptionType)
				.add(iv)
				.add(cipher);
		std::string data = packet.get();
		int length = static_cast<int>(data.size());

		// send packet
		if (udp.sendmsg(data.c_str(), length) == length)
			std::cout << "Telemetry packet sent successfully!" << std::endl;

		usleep(200000); // 5 Hz
	}

	// end communication and flight
	communicator.end(flightID);
	communicator.end_flight(flightID);

	return 0;
}
