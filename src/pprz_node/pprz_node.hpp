#pragma once

#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "nng_event_loop/event_loop.hpp"
#include "nng_event_loop/udp_source.hpp"
#include "nng_event_loop/publisher.hpp"
#include "nng_event_loop/subscriber.hpp"
#include "nng_event_loop/requester.hpp"

#include "vutura_common.pb.h"

enum utm_state_t {
	UTM_STATE_INIT = 0,
	UTM_STATE_LOGGED_IN,
	UTM_STATE_REQUEST_PENDING,
	UTM_STATE_REQUEST_REJECTED,
	UTM_STATE_REQUEST_APPROVED,
	UTM_STATE_FLIGHT_STARTED
};

enum utm_request_t {
	UTM_REQUEST_FLIGHT,
	UTM_REQUEST_START,
	UTM_REQUEST_END
};

union paparazzi_to_vutura_msg_t {
	struct {
		int32_t lon;
		int32_t lat;
		int32_t alt;
		int32_t Vn;
		int32_t Ve;
		int32_t Vd;
		uint32_t target_wp;
	};
	unsigned char bytes;
} __attribute((__packed__));
typedef union paparazzi_to_vutura_msg_t PaparazziToVuturaMsg;

union vutura_to_paparazzi_msg_t {
        struct  {
                bool avoid;
		int32_t vn;
		int32_t ve;
		int32_t vd;
		int32_t lat; //[dege7]
		int32_t lon; //[dege7]
        };
	unsigned char bytes;
} __attribute((__packed__));
typedef union vutura_to_paparazzi_msg_t VuturaToPaparazziMsg;

union paparazzi_to_utm_interface_t {
	struct {
		int32_t utm_request;
	};
	unsigned char bytes;
} __attribute((__packed__));
typedef union paparazzi_to_utm_interface_t PaparazziToUtmInterfaceMsg;

union utm_interface_to_paparazzi_t {
	struct {
		int32_t utm_state;
	};
	unsigned char bytes;
} __attribute((__packed__));
typedef union utm_interface_to_paparazzi_t UtmInterfaceToPaparazziMsg;


class PaparazziNode : public EventLoop {
public:
	PaparazziNode(int instance);
	~PaparazziNode();

	void init();
	void pprz_callback(std::string packet);
	void avoidance_command_callback(std::string message);
	void utm_interface_callback(std::string packet);
	void utm_status_callback(std::string status);
	void utm_sp_receive_callback(std::string reply);

private:
	int _instance;

	UdpSource _pprz_comm;
	Subscriber _avoidance_sub;
	Publisher _position_publisher;
	UdpSource _utm_interface;

	Subscriber _utm_status_sub;
	std::string _utm_status;
	Requester _utm_sp_req;

	PaparazziToVuturaMsg _gpos_msg;

	uint8_t _buffer[BUFFER_LENGTH];

        int handle_avoidance(AvoidanceVelocity avoidance_velocity);
};
