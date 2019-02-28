#pragma once

#include "event_source.hpp"


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

