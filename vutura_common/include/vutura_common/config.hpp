#pragma once
#include <string>

#define MAVLINK_SYSTEM_ID 255
#define MAVLINK_IP			"127.0.0.1"
#define MAVLINK_PORT			14557
#define PPRZ_IP				"127.0.0.1"
#define PPRZ_PORT			8200

#define SOCK_PUBSUB_UTM_STATUS_UPDATE	"ipc:///tmp/utm_status_update"
#define SOCK_PUBSUB_UAV_COMMAND		"ipc:///tmp/uav_command"
#define SOCK_REQREP_AVOIDANCE_COMMAND	"ipc:///tmp/avoidance_command"
#define SOCK_PUBSUB_GPS_POSITION	"ipc:///tmp/gps_position"
#define SOCK_PUBSUB_TRAFFIC_INFO	"tcp://10.11.0.1:8340"
#define SOCK_PUBSUB_UAV_STATUS		"ipc:///tmp/uav_status"

#define SOCK_REQREP_UTMSP_COMMAND       "ipc:///tmp/utmsp"

#define SOCK_PUBSUB_GPS_JSON		"ipc:///tmp/gps_json"
#define SOCK_SUB_DIRECT_MAV_COMMAND	"ipc:///tmp/direct_mav_command_sub"
