#pragma once

#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "nng_event_loop/event_loop.hpp"
#include "nng_event_loop/udp_source.hpp"
#include "nng_event_loop/publisher.hpp"
#include "nng_event_loop/subscriber.hpp"

#include "vutura_common.pb.h"

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

class PaparazziNode : public EventLoop {
public:
	PaparazziNode(int instance);
	~PaparazziNode();

	void init();
	void pprz_callback(std::string packet);
	void avoidance_command_callback(std::string message);


private:
	int _instance;

	UdpSource _pprz_comm;
	Subscriber _avoidance_sub;
	Publisher _position_publisher;

	PaparazziToVuturaMsg _gpos_msg;

	uint8_t _buffer[BUFFER_LENGTH];

        int handle_avoidance(AvoidanceVelocity avoidance_velocity);
};
