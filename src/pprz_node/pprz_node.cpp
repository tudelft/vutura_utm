#include "pprz_node.hpp"
#include <unistd.h>
#include <string.h>

#include "vutura_common/subscription.hpp"

PaparazziNode::PaparazziNode(int instance) :
	_instance(instance),
        pprz_comm(this, PPRZ_IP, PPRZ_PORT, pprz_callback),
	position_publisher(socket_name(SOCK_PUBSUB_GPS_POSITION, instance))
{

}

PaparazziNode::~PaparazziNode()
{

}

void PaparazziNode::pprz_callback(EventSource *es)
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

void PaparazziNode::avoidance_command_callback(EventSource *es)
{

        Subscription* avoidance_sub = static_cast<Subscription*>(es);
        PaparazziNode* node = static_cast<PaparazziNode*>(es->_target_object);

        std::string message = avoidance_sub->get_message();
        AvoidanceVelocity avoidance_velocity;
        bool success = avoidance_velocity.ParseFromString(message);
        if (success)
        {
                node->handle_avoidance(avoidance_velocity);
        }

}

int PaparazziNode::handle_avoidance(AvoidanceVelocity avoidance_velocity)
{
        VuturaToPaparazziMsg msg;
        msg.avoid = avoidance_velocity.avoid();
        msg.vn = avoidance_velocity.vn();
        msg.ve = avoidance_velocity.ve();
        msg.vd = avoidance_velocity.vd();

        std::cout << "TEST: " << msg.vn << std::endl;

        sendto(pprz_comm._fd, &msg, sizeof(msg), 0, (struct sockaddr*)&pprz_comm.uav_addr, sizeof(struct sockaddr_in));

        return 0;
}
