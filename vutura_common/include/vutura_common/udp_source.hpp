#pragma once
#include <arpa/inet.h>

#include <nng/nng.h>


#include "event_source.hpp"

#define BUFFER_LENGTH 2041

class UdpSource : public EventSource {
public:
	UdpSource(void* node, std::string address, uint16_t port, void(*cb)(EventSource*));
	~UdpSource();

	ssize_t send_buffer(ssize_t len);

	struct sockaddr_in uav_addr;
	struct sockaddr_in loc_addr;

	uint8_t buf[BUFFER_LENGTH];

private:
	std::string _url;
};

