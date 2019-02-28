#include <unistd.h> // close
#include "timer.hpp"

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

Timer::~Timer()
{
	close(_fd);
}
