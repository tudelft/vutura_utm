#pragma once

#include "event_source.hpp"

typedef void (*sigterm_event_handler_t) (int);

class EventLoop
{
public:
	EventLoop();
	int add(EventSource &source);
	int add_sigterm_event_handler(sigterm_event_handler_t event_handler);
	void start();
	void stop();

	static void sigterm_handler(int param);


private:
	std::vector<EventSource*> _event_sources;
	std::vector<sigterm_event_handler_t> _sigterm_event_handlers;
	bool _should_exit;
};
