#ifndef UNIFLY_CONFIG_HPP
#define UNIFLY_CONFIG_HPP

#include <iostream>

class UniflyConfig {
public:
	UniflyConfig();
	UniflyConfig(std::string filename);
	~UniflyConfig();

	std::string username() { return _username; }
	std::string password() { return _password; }
	std::string client_id() { return _client_id; }
	std::string secret() { return _secret; }
	std::string host() { return _host; }
	uint16_t port() { return _port; }

private:
	std::string _username;
	std::string _password;
	std::string _client_id;
	std::string _secret;
	std::string _host;
	uint16_t _port;

};

#endif // UNIFLY_CONFIG_HPP
