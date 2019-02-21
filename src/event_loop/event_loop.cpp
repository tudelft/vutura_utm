#include <sys/stat.h>
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
				_event_sources.at(i)->_callback(_event_sources.at(i));
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

Subscription::Subscription(std::string url, void(*cb)(EventSource*)) :
	EventSource(cb),
	_url(url)
{
	int rv;
	// Listen to position data
	if ((rv = nng_sub0_open(&_socket)) != 0) {
		fatal("nng_sub0_open", rv);
	}

	if ((rv = nng_setopt(_socket, NNG_OPT_SUB_SUBSCRIBE, "", 0)) != 0) {
		fatal("nng_setopt", rv);
	}

	if ((rv = nng_dial(_socket, _url.c_str(), NULL, NNG_FLAG_NONBLOCK))) {
		fatal("nng_listen", rv);
	}

	if ((rv = nng_getopt_int(_socket, NNG_OPT_RECVFD, &_fd))) {
		fatal("nng_getopt", rv);
	}
}

void Subscription::fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}
