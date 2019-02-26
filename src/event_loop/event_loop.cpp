#include <sys/stat.h>
#include "event_loop.hpp"
#include <nng/protocol/pubsub0/sub.h>
#include <nng/protocol/reqrep0/rep.h>


void fatal(const char *func, int rv)
{
	fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
	exit(1);
}

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

	std::cout << std::endl << "Loop closed" << std::endl;
}

void EventLoop::stop()
{
	_should_exit = true;
}

Subscription::Subscription(void *node, std::string url, void(*cb)(EventSource *)) :
	EventSource(node, -1, cb),
	_url(url)
{
	int rv;

	if ((rv = nng_sub0_open(&_socket)) != 0) {
		fatal("nng_sub0_open", rv);
	}

	if ((rv = nng_setopt(_socket, NNG_OPT_SUB_SUBSCRIBE, "", 0)) != 0) {
		fatal("nng_setopt", rv);
	}

	if ((rv = nng_dial(_socket, _url.c_str(), NULL, NNG_FLAG_NONBLOCK))) {
		fatal("nng_listen", rv);
	}

	// Set re-connect time to 500ms, helps when starting subscriber first
	if ((rv = nng_setopt_ms(_socket, NNG_OPT_RECONNMINT, 500)) != 0) {
		fatal("nng_setopt_ms", rv);
	}

	if ((rv = nng_setopt_ms(_socket, NNG_OPT_RECONNMAXT, 0)) != 0) {
		fatal("nng_setopt_ms", rv);
	}

	if ((rv = nng_getopt_int(_socket, NNG_OPT_RECVFD, &_fd))) {
		fatal("nng_getopt", rv);
	}
}

Replier::Replier(void *node, std::string url, void (*cb)(EventSource *)) :
	EventSource(node, -1, cb),
	_url(url)
{
	int rv;
	// Configure reply topic for utmsp request
	if ((rv = nng_rep0_open(&_socket)) != 0) {
		fatal("nng_rep0_open", rv);
	}

	if ((rv = nng_listen(_socket, _url.c_str(), NULL, NNG_FLAG_NONBLOCK))) {
		fatal("nng_listen", rv);
	}

	if ((rv = nng_getopt_int(_socket, NNG_OPT_RECVFD, &_fd))) {
		fatal("nng_getopt", rv);
	}
}

Timer::Timer(void *node, uint32_t interval_ms, void (*cb)(EventSource *)) :
	EventSource(node, -1, cb)
{
	struct timespec ts;
	ts.tv_sec = interval_ms / 1000;
	ts.tv_nsec = (interval_ms % 1000) * 1e6;
	struct itimerspec its;
	its.it_interval = ts;
	its.it_value = ts;

	_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timerfd_settime(_fd, 0, &its, NULL) == -1) {
		std::cerr << "ERROR setting periodic timer" << std::endl;
	}
}
