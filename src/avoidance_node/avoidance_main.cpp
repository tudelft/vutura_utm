#include "vutura_common/config.hpp"
#include "vutura_common/event_loop.hpp"
#include "vutura_common/timer.hpp"
#include "vutura_common/subscription.hpp"

#include "avoidance_node.hpp"

int main(int argc, char **argv)
{
	int instance = 0;
	if (argc > 1) {
		instance = atoi(argv[1]);
	}
	std::cout << "Instance " << std::to_string(instance) << std::endl;

	AvoidanceNode node(instance);
	EventLoop event_loop;

	Timer periodic_timer(&node, 200, node.periodic_timer_callback);
	event_loop.add(periodic_timer);

	Subscription traffic_sub(&node, socket_name(SOCK_PUBSUB_TRAFFIC_INFO, instance), node.traffic_callback);
	event_loop.add(traffic_sub);

	Subscription gps_sub(&node, socket_name(SOCK_PUBSUB_GPS_POSITION, instance), node.gps_position_callback);
	event_loop.add(gps_sub);

//	double x,y,dist;
//	node.get_relative_coordinates(52.17164192042854, 4.421846866607666, 52.174050125852816, 4.425022602081299, &x, &y);
//	dist = sqrt(x*x + y*y);
//	std::cout << "x: " << std::to_string(x) << "\ty: " << std::to_string(y) << "\tdist: " << std::to_string(dist) <<  std::endl;

	event_loop.start();
}
