#pragma once
#include <functional>

#include "experimental_loop.hpp"
#include "better_timer.hpp"

class ExperimentalNode : public ExperimentalLoop
{
public:
	ExperimentalNode(int instance);
	~ExperimentalNode();

	int construct_timer(int interval_ms);

	void print_periodic_message();
	void init();

private:
	int _instance;

	BetterTimer _timer;


};
