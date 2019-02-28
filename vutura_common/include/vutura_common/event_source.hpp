#pragma once

#include <iostream>
#include <vector>
#include <poll.h>
#include <sys/timerfd.h>
#include <nng/nng.h>

class EventSource
{
public:
	EventSource(void* node, int fd, void(*cb)(EventSource*)) :
		_target_object(node),
		_fd(fd),
		_callback(cb)
	{

	}

	void set_target_object(void* target_object) {
		_target_object = target_object;
	}

	bool has_target_object() {
		return _target_object != nullptr;
	}

	void *_target_object;
	int _fd;
	void (*_callback)(EventSource*);

        static void fatal(std::string const& function, const int error)
        {
            std::cerr << function << ": " << nng_strerror(error) << std::endl;
        }
};
