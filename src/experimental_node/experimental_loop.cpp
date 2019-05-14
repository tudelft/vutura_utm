#include <iostream>
#include <poll.h>
#include "experimental_loop.hpp"


ExperimentalLoop::ExperimentalLoop()
{

}

void ExperimentalLoop::add_event_generator(EventGenerator *eg)
{
	if (eg->callback == nullptr) {
		std::cerr << "Callback not defined" << std::endl;
	} else {
		_event_sources.push_back(eg);
	}
}

ExperimentalLoop::~ExperimentalLoop()
{
	std::cout << "ExperimentalLoop destructor" << std::endl;
}

void ExperimentalLoop::run()
{
	std::cout << "RUN" << std::endl;

	// construct fds
	int len = _event_sources.size();
	struct pollfd fds[len];

	for (ssize_t i = 0; i < len; i++) {
		fds[i].fd = _event_sources.at(i)->fd;
		fds[i].events = POLLIN;
		fds[i].revents = 0;
	}

	while (true) {

		int ret = poll(fds, len, 2000);

		if (ret == 0) {
			std::cout << "No event for 2000ms" << std::endl;
			continue;
		}

		// check which event it was, and trigger the callback
		for (ssize_t i = 0; i < len; i++) {
			if (fds[i].revents & POLLIN) {
				_event_sources.at(i)->callback();
				continue;
			}
		}
	}

	std::cout << std::endl << "Loop closed" << std::endl;
}
