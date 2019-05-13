#include <signal.h>
#include <sys/stat.h>
#include "event_loop.hpp"

namespace event_loop {
	EventLoop* object = nullptr;
	bool sigterm_received = false;
	int sigterm_param = 0;
}

EventLoop::EventLoop() :
	_should_exit(false)
{
	if (event_loop::object != nullptr) {
		std::cerr << "Event loop already exists" << std::endl;
		exit(EXIT_FAILURE);
	}
	event_loop::object = this;
	signal(SIGINT, sigterm_handler);
	signal(SIGTERM, sigterm_handler);
}

int EventLoop::add(EventSource &source)
{
	_event_sources.push_back(&source);
}

int EventLoop::add_sigterm_event_handler(sigterm_event_handler_t event_handler)
{
	_sigterm_event_handlers.push_back(event_handler);
}

void EventLoop::start()
{
	// construct fds
	int len = _event_sources.size();
	struct pollfd fds[len];

	for (ssize_t i = 0; i < len; i++) {
		fds[i].fd = _event_sources.at(i)->_fd;
		fds[i].events = POLLIN;
		fds[i].revents = 0;
	}

	while (true) {
		if (event_loop::sigterm_received) {
			// send sigterm event to callbacks
			for (ssize_t i = 0; i < _sigterm_event_handlers.size(); i++) {
				//std::cout << "sigterm event handler calling" << std::endl;
				_sigterm_event_handlers.at(i)(event_loop::sigterm_param);
			}

			// break the loop, this ends the program
			break;
		}
		int ret = poll(fds, len, 2000);

		if (ret == 0) {
			std::cout << "No event for 2000ms" << std::endl;
			continue;
		}

		// check which event it was, and trigger the callback
		for (ssize_t i = 0; i < len; i++) {
			if (fds[i].revents & POLLIN) {
				_event_sources.at(i)->_callback(_event_sources.at(i));
				continue;
			}
		}
	}

	std::cout << std::endl << "Loop closed" << std::endl;
}

void EventLoop::stop()
{
	_should_exit = true;
}

void EventLoop::sigterm_handler(int param)
{
	if (event_loop::sigterm_received) {
		std::cerr << "Already received a sigterm signal" << std::endl;
		exit(EXIT_FAILURE);
	}
	std::cout << "[event_loop] SIGTERM/SIGINT" << std::endl;
	event_loop::sigterm_received = true;
	event_loop::sigterm_param = param;
}
