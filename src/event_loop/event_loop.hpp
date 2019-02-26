#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <iostream>
#include <vector>
#include <poll.h>
#include <sys/timerfd.h>
#include <nng/nng.h>


class EventSource
{
public:
	EventSource(void* node, int fd, void(*cb)(EventSource*)) :
		_target_object(node),
		_fd(fd),
		_callback(cb)
	{

	}

	void set_target_object(void* target_object) {
		_target_object = target_object;
	}

	bool has_target_object() {
		return _target_object != nullptr;
	}

	void *_target_object;
	int _fd;
	void (*_callback)(EventSource*);
};

class Subscription : public EventSource {
public:
	Subscription(void* node, std::string topic, void(*cb)(EventSource *));

	nng_socket _socket;

private:
	std::string _url;
};

class Replier : public EventSource {
public:
	Replier(void* node, std::string url, void(*cb)(EventSource*));

	nng_socket _socket;

private:
	std::string _url;
};

class Timer: public EventSource {
public:
	Timer(void *node, uint32_t interval_ms, void(*cb)(EventSource*));

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
