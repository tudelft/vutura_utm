#include <unistd.h> // close
#include <iostream>
#include <sys/timerfd.h>

#include "better_timer.hpp"

BetterTimer::BetterTimer(uint32_t interval_ms)
{
	struct timespec ts;
	ts.tv_sec = interval_ms / 1000;
	ts.tv_nsec = (interval_ms % 1000) * 1e6;
	struct itimerspec its;
	its.it_interval = ts;
	its.it_value = ts;

	fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timerfd_settime(fd, 0, &its, NULL) == -1) {
		std::cerr << "ERROR setting periodic timer" << std::endl;
	}

}

BetterTimer::~BetterTimer()
{
	close(fd);
}

void BetterTimer::on_timeout(std::function<void ()> cb)
{
	_cb = cb;
}

void BetterTimer::pollin_event()
{
	uint64_t num_timer_events;
	ssize_t recv_size = read(fd, &num_timer_events, 8);
	(void) recv_size;
	if (_cb == nullptr) {
		std::cerr << "No callback defined!" << std::endl;
	} else {
		_cb();
	}
}
