#include "airmap_traffic.hpp"
#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"

AirmapTraffic::AirmapTraffic(int instance) :
	_client(std::string("ssl://") + AIRMAP_TRAFF_HOST + std::string(":") + std::to_string(AIRMAP_TRAFF_PORT), airmap::device_id),
	_cb(nullptr)
{
	_conn_opts.set_keep_alive_interval(20);
	_conn_opts.set_clean_session(true);

	// Open nng socket for publishing traffic info
	int rv;

	if ((rv = nng_pub0_open(&_pub_traffic)) != 0) {
		std::cerr << "nng_pub0_open pub traffic: " << nng_strerror(rv) << std::endl;
	}

	if ((rv = nng_listen(_pub_traffic, socket_name(SOCK_PUBSUB_TRAFFIC_INFO, instance).c_str(), NULL, 0)) != 0) {
		std::cerr << "nng_listen pub traffic: " << nng_strerror(rv) << std::endl;
	}
}

AirmapTraffic::~AirmapTraffic()
{
	stop();
}

int AirmapTraffic::start(const std::string& flightID, const std::string& token)
{
	if (_cb != nullptr) {
		std::cerr << "_cb is not null" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Connecting to: " << _client.get_server_uri() << std::endl;

	_conn_opts.set_user_name(flightID);
	_conn_opts.set_password(token);

	std::cout << "with user:     " << _conn_opts.get_user_name() << std::endl;
	std::cout << "with password: " << _conn_opts.get_password_str() << std::endl;
	//_sslopts.set_trust_store("/home/bart/git/paho.mqtt.c/test/tls-testing/keys/test-root-ca.crt");
	_sslopts.set_enable_server_cert_auth(false);
	std::cout << "verify enabled? " << (_sslopts.get_enable_server_cert_auth() ? "YES" : "NO") << std::endl;

	_conn_opts.set_ssl(_sslopts);

	_cb = new AirmapTrafficCallback(_client, _conn_opts, flightID, _pub_traffic);
	_client.set_callback(*_cb);

	try {
		std::cout << "Connecting to MQTT server..." << std::endl;
		_client.connect(_conn_opts, nullptr, *_cb);
	} catch (const mqtt::exception&) {
		std::cerr << "Unable to connect to MQTT server" << std::endl;
		return -1;
	}

	return 0;
}

int AirmapTraffic::stop()
{
	std::cout << "Disconnecting MQTT" << std::endl;
	_client.disconnect(3000);

	if (_cb != nullptr) {
		delete _cb;
		_cb = nullptr;
	}
}
