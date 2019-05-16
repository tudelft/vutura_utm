#pragma once

#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "vutura_common/udp_source.hpp"
#include "vutura_common/publisher.hpp"
#include "vutura_common.pb.h"

union paparazzi_to_vutura_msg_t {
	struct {
		int32_t lon;
		int32_t lat;
		int32_t alt;
		int32_t Vn;
		int32_t Ve;
		int32_t Vd;
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
        };
        unsigned char bytes;
} __attribute((__packed__));
typedef union vutura_to_paparazzi_msg_t VuturaToPaparazziMsg;

class PaparazziNode {
public:
	PaparazziNode(int instance);
	~PaparazziNode();

        static void pprz_callback(EventSource* es);
        static void avoidance_command_callback(EventSource* es);

        UdpSource pprz_comm;
	Publisher position_publisher;

private:
	int _instance;

	PaparazziToVuturaMsg _gpos_msg;

        int handle_avoidance(AvoidanceVelocity avoidance_velocity);
};
