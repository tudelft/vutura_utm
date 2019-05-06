#include <fstream>

// json -- https://github.com/nlohmann/json
#include <nlohmann/json.hpp>

#include "avoidance_config.hpp"

Avoidance_config::Avoidance_config() :
    _t_lookahead(AVOIDANCE_T_LOOKAHEAD),
    _t_pop_traffic(AVOIDANCE_T_POP_TRAFFIC)
    {

    }

int Avoidance_config::parse_config(std::string config_file)
{
	nlohmann::json config;
	try {
		std::ifstream i(config_file);
		i >> config;
		_t_lookahead = config["t_lookahead"];
        _t_pop_traffic = config["t_pop_traffic"];
	} catch (...) {
		std::cerr << "Failed to parse config file " << config_file << std::endl;
		return -1;
	}
	std::cout << config.dump(4) << std::endl;
	return 0;
}

double Avoidance_config::getTPopTraffic()
{
    return _t_pop_traffic;
}
