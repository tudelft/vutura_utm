#include <sys/timerfd.h>
#include <iostream>
#include <unistd.h>
#include <functional>

#include "experimental_node.hpp"

ExperimentalNode::ExperimentalNode(int instance) :
	_instance(instance),
	_timer(1000)
{

}

ExperimentalNode::~ExperimentalNode()
{

}

void ExperimentalNode::print_periodic_message()
{
	std::cout << "Periodic message" << std::endl;
}

void ExperimentalNode::init()
{	
	_timer.on_timeout(std::bind(&ExperimentalNode::print_periodic_message, this));

	add_event_generator(&_timer);
}
