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
	_utm_interface(this),
	_utm_status_sub(this),
	_utm_status(""),
	_utm_sp_req(this, socket_name(SOCK_REQREP_UTMSP_COMMAND, instance))
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

	_utm_interface.set_receive_callback(std::bind(&PaparazziNode::utm_interface_callback, this, _1));
	_utm_interface.connect(PPRZ_IP, PPRZ_PORT+1, PPRZ_PORT+101);

	_utm_status_sub.set_receive_callback(std::bind(&PaparazziNode::utm_status_callback, this, _1));
	_utm_status_sub.subscribe(socket_name(SOCK_PUBSUB_UTM_STATUS_UPDATE, _instance));

	_utm_sp_req.set_receive_callback(std::bind(&PaparazziNode::utm_sp_receive_callback, this, _1));
}

void PaparazziNode::pprz_callback(std::string packet)
{
//	std::cout << sizeof(PaparazziToVuturaMsg) << " Got UDP data with size: " << packet.length() << std::endl;

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
		msg.set_wind_north(_gpos_msg.wind_north);
		msg.set_wind_east(_gpos_msg.wind_east);
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

void PaparazziNode::utm_interface_callback(std::string packet)
{
	// handle request, used for request, start and end flight
	PaparazziToUtmInterfaceMsg msg;
	if (packet.length() == sizeof(PaparazziToUtmInterfaceMsg)) {
		memcpy((char*)&msg, packet.c_str(), packet.length());

		std::cout << "Got request: " << std::to_string(msg.utm_request) << std::endl;

		std::string request = "";
		switch (msg.utm_request) {
		case UTM_REQUEST_FLIGHT:
			request = "request_flight";
			break;

		case UTM_REQUEST_START:
			request = "start_flight";
			break;

		case UTM_REQUEST_END:
			request = "end_flight";
			break;

		default:
			break;
		}

		if (request != "") {
			_utm_sp_req.request(request);
		}
	}
}

void PaparazziNode::utm_status_callback(std::string status)
{
	if (_utm_status != status) {
		_utm_status = status;
		std::cout << "UTM status update: " << status << std::endl;
	}
	// Also do something with it.
	// For example, switch flight plan block to notify user of authorized/rejected flight
	UtmInterfaceToPaparazziMsg msg;
	msg.utm_state = -1;

	if (status == "logged in") {
		msg.utm_state = UTM_STATE_LOGGED_IN;

	} else if (status == "flight requested") {
		msg.utm_state = UTM_STATE_REQUEST_PENDING;

	} else if (status == "flight authorized") {
		msg.utm_state = UTM_STATE_REQUEST_APPROVED;

	} else if (status == "flight started") {
		msg.utm_state = UTM_STATE_FLIGHT_STARTED;

	}

	if (msg.utm_state != -1) {
		std::string packet((char*)&msg, sizeof(msg));
		_utm_interface.send_packet(packet);
	}

	// Some of the possible states are:
	// "logged in"			Can do the request now, also for rejected
	// "flight requested"		Waiting for permission
	// "flight authorized"		Flight authorized
	// "flight started"		Streaming position
	// "flight rescinded"		ATC wants you to land
	// "flight cancelled"		ATC landing request was confirmed
}

void PaparazziNode::utm_sp_receive_callback(std::string reply)
{

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
	msg.skip_wp = avoidance_velocity.skip_wp();
	msg.skip_to_wp = avoidance_velocity.skip_to_wp();

	std::string packet((char*)&msg, sizeof(msg));
	_pprz_comm.send_packet(packet);

        return 0;
}
