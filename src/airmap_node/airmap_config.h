#pragma once
#include <string>

#define AIRMAP_HOST "api.airmap.com"

#define AIRMAP_SSO_HOST "sso.airmap.io"
#define AIRMAP_SSO_PORT 443

#define AIRMAP_TELEM_HOST "telemetry.airmap.com"
#define AIRMAP_TELEM_PORT 16060

#define AIRMAP_TRAFF_HOST "mqtt-prod.airmap.io"
#define AIRMAP_TRAFF_PORT 8883

#define AIRMAP_API_KEY ""
#define AIRMAP_CLIENT_ID ""
#define AIRMAP_DEVICE_ID ""
#define AIRMAP_USERNAME ""
#define AIRMAP_PASSWORD ""

namespace airmap {
	extern std::string username;
	extern std::string password;
	extern std::string api_key;
	extern std::string client_id;
	extern std::string device_id;
} // namespace airmap
