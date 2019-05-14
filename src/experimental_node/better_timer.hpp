#pragma once
#include <functional>
#include "event_generator.hpp"

class BetterTimer : public EventGenerator
{
public:
	BetterTimer(uint32_t interval_ms);
	~BetterTimer();

	void on_timeout(std::function<void(void)> cb);

	void pollin_event();

private:
	std::function<void(void)> _cb = nullptr;
};

