#include <iostream>
#include <unistd.h>
#include <sys/timerfd.h>
#include <signal.h>
#include "event_loop.hpp"

void timer_callback(EventSource* es)
{
	uint64_t num_timer_events;
	ssize_t recv_size = read(es->_fd, &num_timer_events, 8);
	(void) recv_size;

	std::cout << "timer event " << es->_fd << std::endl;
}

class TestClass {
public:
	TestClass() {
	}

	void run() {

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

		EventSource src(nullptr, timerfd, &timer_callback);
		_loop.add(src);

		_loop.start();
	}

	void stop() {
		_loop.stop();
	}

	static void test_callback(void* me, EventSource* es) {
		std::cout << "test_callback " << std::endl;
	}
private:
	EventLoop _loop;
};

TestClass testclass;

void exit_handler(int s)
{
	testclass.stop();
}

int main()
{
	signal(SIGINT, exit_handler);

	testclass.run();
}
