#include <fstream>

#include <nlohmann/json.hpp>

#include "unifly_config.hpp"

UniflyConfig::UniflyConfig()
{

}

UniflyConfig::UniflyConfig(std::string filename)
{
	nlohmann::json config;
	try {
		std::ifstream i(filename);
		i >> config;
		_username = config["username"];
		_password = config["password"];
		_client_id = config["client_id"];
		_secret = config["secret"];
		_host = config["host"];
		_port = config["port"];

	} catch (...) {
		std::cerr << "Failed to parse config file " << filename << std::endl;
	}
}

UniflyConfig::~UniflyConfig()
{

}
