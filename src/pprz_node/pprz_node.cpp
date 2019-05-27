#include <unistd.h>
#include <string.h>
#include <iostream>

#include "pprz_node.hpp"

using namespace std::placeholders;

PaparazziNode::PaparazziNode(int instance) :
	_instance(instance),
	_pprz_comm(this),
	_avoidance_sub(this),
	_position_publisher(socket_name(SOCK_PUBSUB_GPS_POSITION, instance)),
	_utm_status_sub(this),
	_utm_status("")
{

}

PaparazziNode::~PaparazziNode()
{

}

void PaparazziNode::init()
{
	_pprz_comm.set_receive_callback(std::bind(&PaparazziNode::pprz_callback, this, _1));
	_pprz_comm.connect(PPRZ_IP, PPRZ_PORT, 14551);

	_avoidance_sub.set_receive_callback(std::bind(&PaparazziNode::avoidance_command_callback, this, _1));
	_avoidance_sub.subscribe(socket_name(SOCK_REQREP_AVOIDANCE_COMMAND, _instance));

	_utm_status_sub.set_receive_callback(std::bind(&PaparazziNode::utm_status_callback, this, _1));
	_utm_status_sub.subscribe(socket_name(SOCK_PUBSUB_UTM_STATUS_UPDATE, _instance));
}

void PaparazziNode::pprz_callback(std::string packet)
{
	std::cout << "Got UDP data with size: " << packet.length() << std::endl;

	if (packet.length() == sizeof(PaparazziToVuturaMsg)) {
		memcpy((char*)&_gpos_msg, packet.c_str(), packet.length());
		GPSMessage msg;
		msg.set_timestamp(0);
		msg.set_lat(_gpos_msg.lat);
		msg.set_lon(_gpos_msg.lon);
		msg.set_alt_agl(_gpos_msg.alt);
		msg.set_alt_msl(_gpos_msg.alt);
		msg.set_vn(_gpos_msg.Vn);
		msg.set_ve(_gpos_msg.Ve);
		msg.set_vd(_gpos_msg.Vd);
		msg.set_target_wp(_gpos_msg.target_wp);
		std::string gps_message = msg.SerializeAsString();
		_position_publisher.publish(gps_message);
	}
}

void PaparazziNode::avoidance_command_callback(std::string message)
{
	AvoidanceVelocity avoidance_velocity;
	bool success = avoidance_velocity.ParseFromString(message);
	if (success)
	{
		handle_avoidance(avoidance_velocity);
	}
}

void PaparazziNode::utm_status_callback(std::string status)
{
	if (_utm_status != status) {
		_utm_status = status;
		std::cout << "UTM status update: " << status << std::endl;

		// Also do something with it.
		// For example, switch flight plan block to notify user of authorized/rejected flight

		// Some of the possible states are:
		// "logged in"			Can do the request now, also for rejected
		// "flight requested"		Waiting for permission
		// "flight authorized"		Flight authorized
		// "flight started"		Streaming position
		// "flight rescinded"		ATC wants you to land
		// "flight cancelled"		ATC landing request was confirmed
	}
}

int PaparazziNode::handle_avoidance(AvoidanceVelocity avoidance_velocity)
{
        VuturaToPaparazziMsg msg;
        msg.avoid = avoidance_velocity.avoid();
        msg.vn = avoidance_velocity.vn();
        msg.ve = avoidance_velocity.ve();
        msg.vd = avoidance_velocity.vd();
	msg.lat = avoidance_velocity.lat();
	msg.lon = avoidance_velocity.lon();

	std::string packet((char*)&msg, sizeof(msg));
	_pprz_comm.send_packet(packet);

        return 0;
}
