#include "airmap_traffic.hpp"

AirmapTraffic::AirmapTraffic() :
	_client(std::string("ssl://") + AIRMAP_TRAFF_HOST + std::string(":") + std::to_string(AIRMAP_TRAFF_PORT), AIRMAP_DEVICE_ID),
	_cb(nullptr)
{
	_conn_opts.set_keep_alive_interval(20);
	_conn_opts.set_clean_session(true);
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
	_sslopts.set_trust_store("/home/bart/git/paho.mqtt.c/test/tls-testing/keys/test-root-ca.crt");
	_sslopts.set_enable_server_cert_auth(false);
	std::cout << "verify enabled? " << (_sslopts.get_enable_server_cert_auth() ? "YES" : "NO") << std::endl;

	_conn_opts.set_ssl(_sslopts);

	_cb = new AirmapTrafficCallback(_client, _conn_opts, flightID);
	_client.set_callback(*_cb);

	try {
		std::cout << "Connecting to MQTT server..." << std::endl;
		_client.connect(_conn_opts, nullptr, *_cb);
	} catch (const mqtt::exception&) {
		std::cerr << "Unable to connect to MQTT server" << std::endl;
		return 1;
	}
}

int AirmapTraffic::stop()
{
	_client.disconnect();

	if (_cb != nullptr) {
		delete _cb;
		_cb = nullptr;
	}
}
