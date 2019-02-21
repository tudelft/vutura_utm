#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <iostream>
#include <vector>
#include <poll.h>


class EventSource
{
public:
	EventSource(int fd, void(*cb)(int)) {
		_fd = fd;
		callback = cb;
	}

	int _fd;
	void (*callback)(int);
};

class EventLoop
{
public:
	EventLoop();
	int add(EventSource &source);
	void start();
	void stop();
private:
	std::vector<EventSource*> _event_sources;
	bool _should_exit;
};

#endif // EVENTLOOP_H
