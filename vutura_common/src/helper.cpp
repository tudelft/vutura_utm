#include "helper.hpp"


std::string socket_name(std::string topic, int instance)
{
	return topic + "_" + std::to_string(instance);
}
