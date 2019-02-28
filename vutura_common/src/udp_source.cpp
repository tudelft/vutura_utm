#include <string.h>
#include <unistd.h> // close
#include "udp_source.hpp"


UdpSource::UdpSource(void *node, std::string address, uint16_t port, void (*cb)(EventSource *)) :
	EventSource(node, -1, cb)
{
	// Make a UDP connection
	_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// Listen to UAV
	memset(&loc_addr, 0, sizeof(loc_addr));
	loc_addr.sin_family = AF_INET;
	loc_addr.sin_addr.s_addr = INADDR_ANY;
	loc_addr.sin_port = htons(14551);

	if (bind(_fd, (struct sockaddr *)&loc_addr, sizeof(struct sockaddr)) == -1)
	{
		std::cerr << "UDP bind failed" << std::endl;
		close(_fd);
		exit(EXIT_FAILURE);
	}

	// Configure socket for sending to UAV
	memset(&uav_addr, 0, sizeof(uav_addr));
	uav_addr.sin_family = AF_INET;
	uav_addr.sin_addr.s_addr = inet_addr(address.c_str());
	uav_addr.sin_port = htons(port);
}

UdpSource::~UdpSource()
{
	close(_fd);
}

ssize_t UdpSource::send_buffer(ssize_t len)
{
	// send stuff that is in buf
	ssize_t bytes_sent = sendto(_fd, buf, len, 0, (struct sockaddr*)&uav_addr, sizeof(struct sockaddr_in));
	return bytes_sent;
}
