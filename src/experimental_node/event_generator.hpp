#pragma once
#include <functional>

class EventGenerator {
public:
	EventGenerator();
	virtual ~EventGenerator() = 0;

	virtual void pollin_event();

	int fd;
};
