#pragma once

#include "event_source.hpp"

class Timer: public EventSource {
public:
	Timer(void *node, uint32_t interval_ms, void(*cb)(EventSource*));
	~Timer();
};

