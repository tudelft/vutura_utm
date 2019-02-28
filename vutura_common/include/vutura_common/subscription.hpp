#pragma once

#include <nng/nng.h>

#include "event_source.hpp"

class Subscription : public EventSource {
public:
	Subscription(void* node, std::string topic, void(*cb)(EventSource *));
	~Subscription();

	std::string get_message();

private:
	nng_socket _socket;
	std::string _url;
};

