#pragma once

// Communicator
class AirmapCommunicator
{
public:
	AirmapCommunicator(const std::string& apiKey) {
		m_url = "https://" AIRMAP_HOST;
		m_headers = NULL;
		m_headers = curl_slist_append(m_headers, ("X-API-KEY: " + apiKey).c_str());
		m_authorization_status = "";
		curl_global_init(CURL_GLOBAL_DEFAULT);
	}

	int authenticate() {
		std::cout << "Auth " << airmap::username << std::endl;
		std::string url = "https://" AIRMAP_SSO_HOST "/oauth/ro";
		std::string res;
		std::string post_data = "grant_type=password"
				   "&client_id=" + airmap::client_id +
				"&connection=Username-Password-Authentication"
				"&username=" + airmap::username +
				"&password=" + airmap::password +
				"&scope=openid offline_access"
				"&device=" + airmap::device_id;
		if (CURLE_OK != curl_post(url.c_str(), m_headers, post_data.c_str(), res))
			return -1;
		// parse result to get token
		std::cout << res << std::endl;
		auto j = nlohmann::json::parse(res);
		try {
			m_token = j["id_token"];
			m_headers = curl_slist_append(m_headers, ("Authorization: Bearer " + m_token).c_str());
			m_headers = curl_slist_append(m_headers, "Accept: application/json");
			m_headers = curl_slist_append(m_headers, "Content-Type: application/json");
			m_headers = curl_slist_append(m_headers, "charsets: utf-8");
		}
		catch (...) {
			return -1; // failed authentication!
		}
		return 0;
	}

	int get_pilot_id(std::string& pilotID) {
		std::string url = m_url + "/pilot/v2/profile";
		std::string res;
		if (CURLE_OK != curl_get(url.c_str(), m_headers, res)) {
			return -1;
		}
		std::cout << "request pilot id:" << std::endl << res << std::endl;
		auto j = nlohmann::json::parse(res);
		try {
			pilotID = j["data"]["id"];
		}
		catch (...) {
			return -1;
		}
	}

	int create_flightplan(const float latitude, const float longitude, const nlohmann::json geometry, const std::string& pilotID, std::string& flightplanID) {
		std::string url = m_url + "/flight/v2/plan";

		time_t current_time;
		time(&current_time);
		char now[21];
		strftime(now, 21, "%FT%TZ", gmtime(&current_time));

		time_t land_time = current_time + 30 * 60;
		char end[21];
		strftime(end, 21, "%FT%TZ", gmtime(&land_time));

		nlohmann::json request = {
			{"pilot_id", pilotID},
			{"start_time", now},
			{"end_time", end},
			{"buffer", 20},
			{"max_altitude_agl", 120},
			{"takeoff_latitude", latitude},
			{"takeoff_longitude", longitude},
			{"rulesets", {"custom_z5d341_drone_rules"}}
		};

		if (geometry == nullptr) {
			nlohmann::json geom;
			geom["type"] = "Point";
			geom["coordinates"] = {longitude, latitude};
			request["geometry"] = geom;
		} else {
			request["geometry"] = geometry;
		}

		std::cout << request.dump(4) << std::endl;

		std::string res;
		if (CURLE_OK != curl_post(url.c_str(), m_headers, request.dump().c_str(), res)) {
			return -1;
		}
		// parse result to get flightID
		std::cout << res << std::endl;
		auto j = nlohmann::json::parse(res);
		try {
			flightplanID = j["data"]["id"];
		}
		catch (...) {
			return -1; // failed flight creation!
		}
		return 0;
	}

	int get_flight_briefing(const std::string& flightplanID) {
		std::string url = m_url + "/flight/v2/plan/" + flightplanID + "/briefing";
		std::string res;
		if (CURLE_OK != curl_get(url.c_str(), m_headers, res)) {
			return -1;
		}

		try{
			auto j = nlohmann::json::parse(res);
			if (j["data"]["authorizations"].size() == 0) {
				std::cout << "Briefing result:" << std::endl << j.dump(4) << std::endl;
				if (j["status"] == "error") {
					m_authorization_status = "error with authorizations size zero";
				} else {
					std::cout << "No authorization required, status is: " << j["data"]["status"] << std::endl;
					m_authorization_status = "not required";
				}
			} else {
				//				std::cout << "Authorization request: " << j["data"]["authorizations"][0]["status"] << std::endl;
				m_authorization_status = j["data"]["authorizations"][0]["status"];
			}
			if (flight_authorized()) {
				std::cout << j.dump(4) << std::endl;
			}
			std::cout << "Authorization status: " << m_authorization_status << std::endl;
		}
		catch (...) {
			std::cout << "Error get briefing" << std::endl;
			return -1;
		}
	}

	bool flight_authorized() {
		bool ret = false;
		if (m_authorization_status == "accepted" ||
				m_authorization_status == "not required") {
			ret = true;
		}
		return ret;
	}

	int submit_flight(const std::string& flightplanID, std::string& flightID) {
		std::string url = m_url + "/flight/v2/plan/" + flightplanID + "/submit";
		std::string res;
		if (CURLE_OK != curl_post(url.c_str(), m_headers, "", res)) {
			return -1;
		}
		std::cout << "submit result: " << res << std::endl;

		auto j = nlohmann::json::parse(res);
		try {
			flightID = j["data"]["flight_id"];
		}
		catch (...) {
			return -1;
		}
		return 0;
	}

	int start(std::string flightID, std::string& key) {
		std::string url = m_url + "/flight/v2/" + flightID + "/start-comm";
		std::string res;
		if (CURLE_OK != curl_post(url.c_str(), m_headers, "", res))
			return -1;
		// parse result to get key
		auto j = nlohmann::json::parse(res);
		try {
			key = j["data"]["key"];
		}
		catch (...) {
			return -1; // failed communication!
		}
		return 0;
	}

	void end(std::string flightID) {
		std::string url = m_url + "/flight/v2/" + flightID + "/end-comm";
		std::string res;
		curl_post(url.c_str(), m_headers, "", res);
	}

	void end_flight(std::string flightID) {
		std::string url = m_url + "/flight/v2/" + flightID + "/end";
		std::string res;
		curl_post(url.c_str(), m_headers, "", res);
	}

	void query_telemetry(std::string flightID) {
		std::string url = m_url + "/archive/v1/telemetry/position?flight_id=" + flightID;
		std::string res;
		curl_get(url.c_str(), m_headers, res);
		std::cout << "telemetry positions: " << res << std::endl;
	}

	std::string& get_token() {
		return m_token;
	}

	int end_all_active_flights(std::string pilotID) {
		// must have a pilot id for this

		time_t current_time;
		time(&current_time);
		char now[21];
		strftime(now, 21, "%FT%TZ", gmtime(&current_time));

		std::string url = m_url + "/flight/v2/?pilot_id=" + pilotID + "&end_after=" + now;
		std::cout << "URL: \"" << url << "\"" << std::endl;

		std::string res;
		curl_get(url.c_str(), m_headers, res);

		// parse the results with json interpreter
		std::cout << res << std::endl;
		auto j = nlohmann::json::parse(res);
		try {
			auto active_flights = j["data"]["results"];
			for (int i = 0; i < active_flights.size(); i++) {
				std::cout << "Active flight_id: " << active_flights[i]["id"] << std::endl;
				end_flight(active_flights[i]["id"]);
				std::cout << "Ended" << std::endl;
			}
		}
		catch (...) {
			return -1;
		}

		return 0;
	}

private:
	static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
		data->append((char*)ptr, size * nmemb);
		return size * nmemb;
	}

	unsigned int curl_post(const char* url, const struct curl_slist* headers, const char* postfields, std::string& response) {
		CURLcode res = CURLE_FAILED_INIT;
		CURL *curl = curl_easy_init();

		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, AirmapCommunicator::writeFunction);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
//			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);


			res = curl_easy_perform(curl);
			if (res != CURLE_OK) {
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			}

			curl_easy_cleanup(curl);
		}

		return res;
	}

	unsigned int curl_get(const char* url, const struct curl_slist* headers, std::string& response) {
		CURLcode res = CURLE_FAILED_INIT;
		CURL *curl = curl_easy_init();

		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, AirmapCommunicator::writeFunction);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

			res = curl_easy_perform(curl);
			if (res != CURLE_OK) {
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			}

			curl_easy_cleanup(curl);
		}

		return res;
	}

	struct curl_slist *m_headers;
	std::string m_url;
	std::string m_token;
	std::string m_authorization_status;
};

