#include <fstream>

// json -- https://github.com/nlohmann/json
#include <nlohmann/json.hpp>

#include "avoidance_config.hpp"

Avoidance_config::Avoidance_config() :
    _t_lookahead(AVOIDANCE_T_LOOKAHEAD)
    {

    }

int Avoidance_config::parse_config(std::string config_file)
{
	nlohmann::json config;
	try {
		std::ifstream i(config_file);
		i >> config;
		_t_lookahead = config["t_lookahead"];
	} catch (...) {
		std::cerr << "Failed to parse config file " << config_file << std::endl;
		return -1;
	}
	std::cout << config.dump(4) << std::endl;
	return 0;
}