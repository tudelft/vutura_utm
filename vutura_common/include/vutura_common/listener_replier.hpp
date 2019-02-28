#pragma once

#include <nng/nng.h>

#include "event_source.hpp"

class ListenerReplier : public EventSource {
public:
	ListenerReplier(void* node, std::string url, void(*cb)(EventSource*));
	~ListenerReplier();

	std::string get_message();
	void send_response(const std::string &response);

private:
	nng_socket _socket;
	std::string _url;
};

