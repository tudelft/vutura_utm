#include <iostream>
#include "event_generator.hpp"


EventGenerator::EventGenerator()
{

}

void EventGenerator::pollin_event()
{
	std::cout << "No pollin event defined" << std::endl;
}

EventGenerator::~EventGenerator()
{

}
