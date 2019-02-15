#include <string>
#include <cstdint>
#include <iostream>
#include <unistd.h>
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

// command to build with g++
// g++ -std=c++1y -o "telemetry" telemetry.pb.cc telemetry.cpp -lcurl -lcryptopp -lprotobuf

// simple sawtooth wave simulation
#define BUMP(val, dt, mx, initval) val += dt; if (val > mx) val = initval; return val;

class Simulator
{
public:
    Simulator() {
        init();
    }
    void init() {
	m_lat = 4.4171905517578125;
	m_lon = 52.170365387094016;
        m_agl = 0.0;
        m_msl = 0.0;
        m_horizAccuracy = 0.0;
        m_yaw = 0.0;
        m_pitch = -90.0;
        m_roll = -90.0;
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

    double getLattitude() { BUMP(m_lat, 0.002, 34.02, 34.015802) }
    double getLongtitude() { BUMP(m_lon, 0.002, -118.44, -118.451303) }
    float getAgl() { BUMP(m_agl, 1.0, 100.0, 0.0) }
    float getMsl() { BUMP(m_msl, 1.0, 100.0, 0.0) }
    float getHorizAccuracy() { BUMP(m_horizAccuracy, 1.0, 10.0, 0.0) }
    float getYaw() { BUMP(m_yaw, 1.0, 360.0, 0.0) }
    float getPitch() { BUMP(m_pitch, 1.0, 90.0, -90.0) }
    float getRoll() { BUMP(m_roll, 1.0, 90.0, -90.0) }
    float getVelocityX() { BUMP(m_velocity_x, 1.0, 100.0, 10.0) }
    float getVelocityY() { BUMP(m_velocity_y, 1.0, 100.0, 10.0) }
    float getVelocityZ() { BUMP(m_velocity_z, 1.0, 100.0, 10.0) }
    float getPressure() { BUMP(m_pressure, 0.1, 1013.0, 1012.0) }

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
	m_url = "https://" AIRMAP_SSO_HOST;
        m_headers = NULL;
        m_headers = curl_slist_append(m_headers, ("X-API-KEY: " + apiKey).c_str());
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    int authenticate(std::string user_id) {
	std::string url = m_url + "/oauth/ro";
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
        }
        catch (...) {
            return -1; // failed authentication!
        }
        return 0;
    }

    int create_flight(float latitude, float longitude, std::string& flightID) {
        std::string url = m_url + "/flight/v2/point";
        m_headers = curl_slist_append(m_headers, "Accept: application/json");
        m_headers = curl_slist_append(m_headers, "Content-Type: application/json");
        m_headers = curl_slist_append(m_headers, "charsets: utf-8");
        std::string res;
        if (CURLE_OK != curl_post(url.c_str(), m_headers, ("{\"latitude\":" + std::to_string(latitude) + ",\"longitude\":" + std::to_string(longitude) + "}").c_str(), res))
            return -1;
	// parse result to get flightID
	std::cout << res << std::endl;
        auto j = nlohmann::json::parse(res);
        try {
            flightID = j["data"]["id"];
        }
        catch (...) {
            return -1; // failed flight creation!
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
            if (res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

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
    Payload(Simulator *sim) : m_sim(sim) {}
    void build(Buffer& payload) {
        payload.clear();
        // message types
        enum class Type : std::uint8_t { position = 1, speed = 2, attitude = 3, barometer = 4 };

        // position
        airmap::telemetry::Position position;
        position.set_timestamp(m_sim->getTimeStamp());
        position.set_latitude(m_sim->getLattitude());
        position.set_longitude(m_sim->getLongtitude());
        position.set_altitude_agl(m_sim->getAgl());
        position.set_altitude_msl(m_sim->getMsl());
        position.set_horizontal_accuracy(m_sim->getHorizAccuracy());
        auto messagePosition = position.SerializeAsString();
        payload.add<std::uint16_t>(htons(static_cast<std::uint16_t>(Type::position)))
            .add<std::uint16_t>(htons(messagePosition.size()))
            .add(messagePosition);

        // speed
        airmap::telemetry::Speed speed;
        speed.set_timestamp(m_sim->getTimeStamp());
        speed.set_velocity_x(m_sim->getVelocityX());
        speed.set_velocity_y(m_sim->getVelocityY());
        speed.set_velocity_z(m_sim->getVelocityZ());
        auto messageSpeed = speed.SerializeAsString();
        payload.add<std::uint16_t>(htons(static_cast<std::uint16_t>(Type::speed)))
            .add<std::uint16_t>(htons(messageSpeed.size()))
            .add(messageSpeed);

        // attitude
        airmap::telemetry::Attitude attitude;
        attitude.set_timestamp(m_sim->getTimeStamp());
        attitude.set_yaw(m_sim->getYaw());
        attitude.set_pitch(m_sim->getPitch());
        attitude.set_roll(m_sim->getRoll());
        auto messageAttitude = attitude.SerializeAsString();
        payload.add<std::uint16_t>(htons(static_cast<std::uint16_t>(Type::attitude)))
            .add<std::uint16_t>(htons(messageAttitude.size()))
            .add(messageAttitude);

        // barometer
        airmap::telemetry::Barometer barometer;
        barometer.set_timestamp(m_sim->getTimeStamp());
        barometer.set_pressure(m_sim->getPressure());
        auto messageBarometer = barometer.SerializeAsString();
        payload.add<std::uint16_t>(htons(static_cast<std::uint16_t>(Type::barometer)))
            .add<std::uint16_t>(htons(messageBarometer.size()))
            .add(messageBarometer);
    }

private:
    Simulator *m_sim;
};

// main
int main(int argc, char* argv[])
{
    // User ID
    const std::string userID = "Jt0ZUxsVa62v3CAMIqlir2L1bgSki6ha";
    // API Key
    const std::string apiKey = AIRMAP_API_KEY;
    // latitude
    const float latitude = 4.4171905517578125;
    // longitude
    const float longitude = 52.170365387094016;
    // aes-256-cbc
    const uint8_t encryptionType = 1;

    // authenticate, create flight, and start communications
    Communicator communicator(apiKey);
    if (-1 == communicator.authenticate(userID)) {
        std::cout << "Authentication Failed!" << std::endl;
        return -1;
    }

    std::string flightID;
    if (-1 == communicator.create_flight(latitude, longitude, flightID)) {
        std::cout << "Flight Creation Failed!" << std::endl;
        return -1;
    }

    std::string key;
    if (-1 == communicator.start(flightID, key)) {
        std::cout << "Communication Failed!" << std::endl;
        return -1;
    }

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
    for (int i = 0; i < 100; ++i) {
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
