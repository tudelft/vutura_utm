#pragma once
#include <functional>

class EventGenerator {
public:
	EventGenerator();
	virtual ~EventGenerator() = 0;

	int fd;

	std::function<void(void)> callback = nullptr;
};
