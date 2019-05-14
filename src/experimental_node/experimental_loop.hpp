#pragma once
#include <vector>

#include "event_generator.hpp"

class ExperimentalLoop {
public:
	ExperimentalLoop();
	virtual ~ExperimentalLoop() = 0;

	void add_event_generator(EventGenerator* eg);
	void run();

private:

	std::vector<EventGenerator*> _event_sources;
};
