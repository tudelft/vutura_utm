#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <iostream>
#include <vector>
#include <poll.h>
#include <nng/nng.h>
#include <nng/protocol/pubsub0/sub.h>


class EventSource
{
public:
	EventSource(int fd, void(*cb)(EventSource*)) :
		_source_object(nullptr),
		_target_object(nullptr)
	{
		_fd = fd;
		_callback = cb;
	}

	void set_source_object(void* source_object) {
		_source_object = source_object;
	}

	void set_target_object(void* target_object) {
		_target_object = target_object;
	}

	bool has_source_object() {
		return _source_object != nullptr;
	}

	bool has_target_object() {
		return _target_object != nullptr;
	}

	void *_source_object;
	void *_target_object;
	int _fd;
	void (*_callback)(EventSource*);

protected:
	EventSource(void(*cb)(EventSource*)) :
		_source_object(nullptr),
		_target_object(nullptr)
	{
		_callback = cb;
	}

};

class Subscription : public EventSource {
public:
	Subscription(std::string topic, void(*cb)(EventSource*));
	void fatal(const char *func, int rv);

	nng_socket _socket;

private:
	std::string _url;
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
