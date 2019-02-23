#ifndef AIRMAP_TRAFFIC_HPP
#define AIRMAP_TRAFFIC_HPP

#include "airmap_config.h"
#include "mqtt/async_client.h"

class AirmapTraffic
{
public:
	AirmapTraffic();

	int connect(const std::string &flightID, const std::string &token);
	int disconnect();

private:
	mqtt::async_client _client;
};

#endif // AIRMAP_TRAFFIC_HPP
