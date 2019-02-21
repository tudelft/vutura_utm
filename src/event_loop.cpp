#include "event_loop.hpp"

EventLoop::EventLoop() :
	_should_exit(false)
{

}

int EventLoop::add(EventSource &source)
{
	_event_sources.push_back(&source);
}

void EventLoop::start()
{
	// construct fds
	int len = _event_sources.size();
	std::cout << "number: " << len << std::endl;
	struct pollfd fds[len];

	for (ssize_t i = 0; i < len; i++) {
		fds[i].fd = _event_sources.at(i)->_fd;
		fds[i].events = POLLIN;
		fds[i].revents = 0;
	}

	while (!_should_exit) {
		int ret = poll(fds, len, 2000);

		if (ret == 0) {
			std::cout << "No event for 2000ms" << std::endl;
			continue;
		}

		// check which event it was, and trigger the callback
		for (ssize_t i = 0; i < len; i++) {
			if (fds[i].revents & POLLIN) {
				_event_sources[i]->callback(fds[i].fd);
				continue;
			}
		}
	}

	std::cout << "Loop closed" << std::endl;
}

void EventLoop::stop()
{
	_should_exit = true;
}
