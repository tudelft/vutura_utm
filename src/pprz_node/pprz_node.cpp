#include "pprz_node.hpp"
#include <unistd.h>
#include <string.h>
#include "vutura_common.pb.h"

PaparazziNode::PaparazziNode(int instance) :
	_instance(instance),
	gpos_source(this, PPRZ_IP, PPRZ_PORT, gpos_callback),
	position_publisher(socket_name(SOCK_PUBSUB_GPS_POSITION, instance))
{

}

PaparazziNode::~PaparazziNode()
{

}

void PaparazziNode::gpos_callback(EventSource *es)
{
	std::cout << "Got UDP data" << std::endl;
	UdpSource *udp = static_cast<UdpSource*>(es);
	PaparazziNode *node = static_cast<PaparazziNode*>(udp->_target_object);
	memset(udp->buf, 0, BUFFER_LENGTH);
	ssize_t recv_size = recv(udp->_fd, (void*)&node->_gpos_msg, sizeof(PaparazziToVuturaMsg), 0);
	if (recv_size == 24) {
		GPSMessage msg;
		msg.set_timestamp(0);
		msg.set_lat(node->_gpos_msg.lat);
		msg.set_lon(node->_gpos_msg.lon);
		msg.set_alt_agl(node->_gpos_msg.alt);
		msg.set_alt_msl(node->_gpos_msg.alt);
		msg.set_vn(node->_gpos_msg.Vn);
		msg.set_ve(node->_gpos_msg.Ve);
		msg.set_vd(node->_gpos_msg.Vd);
		node->position_publisher.publish(msg.SerializeAsString());
		std::cout << "lat: " << node->_gpos_msg.lat << "\tlon: " << node->_gpos_msg.lon << std::endl;

		for (ssize_t i = 0; i < recv_size; i++) {

//			if (mavlink_parse_char(MAVLINK_COMM_0, udp->buf[i], &msg, &status)) {
//				mavlink_node_incoming_message(node, &msg);
//			}
		}
	}
}

