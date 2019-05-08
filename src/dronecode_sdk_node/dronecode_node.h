#pragma once
#include "vutura_common/helper.hpp"
#include "vutura_common/config.hpp"
#include "vutura_common/udp_source.hpp"
#include "vutura_common/publisher.hpp"

#include "dronecode_sdk/dronecode_sdk.h"
#include "dronecode_sdk/plugins/mission/mission.h"

using namespace dronecode_sdk;

class DronecodeNode {
public:
	DronecodeNode(int instance);

	void uav_command(std::string command);
	void emit_heartbeat();
        void avoidance_velocity_vector(bool avoid, float vn, float ve, float vd);
	void set_armed_state(bool armed);
	void set_guided_state(bool guided_mode_enabled);
	void enable_offboard(bool offboard);


	Publisher gps_pub;
	Publisher uav_armed_pub;

	static void dronecode_on_discover_callback(uint64_t uuid);
	static void dronecode_on_timeout(uint64_t uuid);
	static void heartbeat_timer_callback(EventSource* es);
	static void uav_command_callback(EventSource* es);
	static void avoidance_command_callback(EventSource* es);

private:
	DronecodeSDK _dc;
	Mission* _mission;

	uint64_t _uuid;
	bool _armed;
	bool _guided_mode;
	bool _avoiding;
};
