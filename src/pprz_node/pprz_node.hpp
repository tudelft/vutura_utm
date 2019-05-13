#pragma once

#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "vutura_common/udp_source.hpp"
#include "vutura_common/publisher.hpp"

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

class PaparazziNode {
public:
	PaparazziNode(int instance);
	~PaparazziNode();

	static void gpos_callback(EventSource* es);

	UdpSource gpos_source;
	Publisher position_publisher;

private:
	int _instance;

	PaparazziToVuturaMsg _gpos_msg;
};
