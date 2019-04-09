#pragma once

#include <nng/nng.h>

#include "event_source.hpp"

class Requester : public EventSource {
public:
	Requester(void* node, const std::string &url, void(*cb)(EventSource*));

        void send_request(std::string const& message);
	std::string get_reply();
	uint32_t req_id();

        static void fatal(std::string const& function, const int error)
        {
            std::cerr << function << ": " << nng_strerror(error) << std::endl;
        }

private:
	nng_dialer _dialer;
        nng_socket _sock;
	uint32_t _req_id;
};

