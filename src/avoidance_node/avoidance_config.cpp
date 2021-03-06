#include <fstream>
#include <iostream>

// json -- https://github.com/nlohmann/json
#include <nlohmann/json.hpp>

#include "avoidance_config.hpp"

Avoidance_config::Avoidance_config() :
	    _t_lookahead(AVOIDANCE_T_LOOKAHEAD),
	    _t_pop_traffic(AVOIDANCE_T_POP_TRAFFIC),
	    _r_pz(AVOIDANCE_RPZ),
	    _r_pz_mar(AVOIDANCE_RPZ_MAR),
	    _r_pz_mar_detect(AVOIDANCE_RPZ_MAR_DETECT),
	    _n_angle(AVOIDANCE_N_ANGLE),
	    _v_min(AVOIDANCE_V_MIN),
	    _v_max(AVOIDANCE_V_MAX),
	    _v_set(AVOIDANCE_V_SET),
	    _logging(AVOIDANCE_LOGGING),
	    _avoid_to_target(AVOIDANCE_AVOID_TO_TARGET),
	    _r_turn(AVOIDANCE_R_TURN)
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
		_r_pz = config["r_pz"];
		_r_pz_mar = config["r_pz_mar"];
		_r_pz_mar_detect = config["r_pz_mar_detect"];
		_n_angle = config["n_angle"];
		_v_min = config["v_min"];
		_v_max = config["v_max"];
		_v_set = config["v_set"];
		_logging = config["logging"];
		_log_prefix = config["log_prefix"];
		_avoid_to_target = config["avoid_to_target"];
		_r_turn = config["r_turn"];
	} catch (...) {
		std::cerr << "Failed to parse config file " << config_file << std::endl;
		return -1;
	}
	std::cout << config.dump(4) << std::endl;
	return 0;
}

double Avoidance_config::getTLookahead()
{
	return _t_lookahead;
}

double Avoidance_config::getTPopTraffic()
{
	return _t_pop_traffic;
}

double Avoidance_config::getRPZ()
{
	return _r_pz;
}

double Avoidance_config::getRPZMar()
{
	return _r_pz_mar;
}

double Avoidance_config::getRPZMarDetect()
{
	return _r_pz_mar_detect;
}

unsigned Avoidance_config::getNAngle()
{
	return _n_angle;
}

double Avoidance_config::getVMin()
{
	return _v_min;
}

double Avoidance_config::getVMax()
{
	return _v_max;
}

double Avoidance_config::getVSet()
{
	return _v_set;
}

bool Avoidance_config::getLogging()
{
	return _logging;
}

std::string Avoidance_config::getLogPrefix()
{
	return _log_prefix;
}

bool Avoidance_config::getAvoidToTarget()
{
	return _avoid_to_target;
}

double Avoidance_config::getRTurn()
{
	return _r_turn;
}
