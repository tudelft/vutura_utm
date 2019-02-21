#include <iostream>
#include <unistd.h>
#include <sys/timerfd.h>
#include <signal.h>
#include "event_loop.hpp"

void timer_callback(int fd)
{
	uint64_t num_timer_events;
	ssize_t recv_size = read(fd, &num_timer_events, 8);
	(void) recv_size;

	std::cout << "timer event " << fd << std::endl;
}

EventLoop loop;

void exit_handler(int s)
{
	loop.stop();
}

int main()
{
	signal(SIGINT, exit_handler);

	// Configure periodic timer for heartbeats
	int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
	struct timespec ts = {0};
	ts.tv_sec = 1;
	struct itimerspec its = {0};
	its.it_interval = ts;
	its.it_value = ts;
	if (timerfd_settime(timerfd, 0, &its, NULL) == -1) {
		perror("setting timer");
	}

	EventSource src(timerfd, &timer_callback);
	loop.add(src);

	loop.start();
}
