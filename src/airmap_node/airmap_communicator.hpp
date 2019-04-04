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
		std::string url = "https://" AIRMAP_SSO_HOST "/oauth/ro";
		std::string res;
		char post_data[] = "grant_type=password"
				   "&client_id=" AIRMAP_CLIENT_ID
				"&connection=Username-Password-Authentication"
				"&username=" AIRMAP_USERNAME
				"&password=" AIRMAP_PASSWORD
				"&scope=openid offline_access"
				"&device=" AIRMAP_DEVICE_ID;
		if (CURLE_OK != curl_post(url.c_str(), m_headers, post_data, res))
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

	int create_flightplan(const float latitude, const float longitude, const std::string& pilotID, std::string& flightplanID) {
		std::string url = m_url + "/flight/v2/plan";

		time_t current_time;
		time(&current_time);
		char now[21];
		strftime(now, 21, "%FT%TZ", gmtime(&current_time));

		time_t land_time = current_time + 30 * 60;
		char end[21];
		strftime(end, 21, "%FT%TZ", gmtime(&land_time));

		nlohmann::json geometry;
		geometry["type"] = "Polygon";
		geometry["coordinates"] = nlohmann::json::array({{
									{4.415044784545898,
									 52.17101023840795
								       },
								       {
									 4.415087699890137,
									 52.16679879572676
								       },
								       {
									 4.415860176086426,
									 52.166772472956495
								       },
								       {
									 4.415946006774902,
									 52.16872031589736
								       },
								       {
									 4.418177604675293,
									 52.16861502926976
								       },
								       {
									 4.417705535888672,
									 52.16669350455227
								       },
								       {
									 4.418907165527344,
									 52.16661453600787
								       },
								       {
									 4.419207572937012,
									 52.16835181161079
								       },
								       {
									 4.420409202575684,
									 52.16658821312859
								       },
								       {
									 4.422554969787598,
									 52.16650924439736
								       },
								       {
									 4.424271583557129,
									 52.16840445526715
								       },
								       {
									 4.4241857528686515,
									 52.166482921455795
								       },
								       {
									 4.425473213195801,
									 52.16643027552595
								       },
								       {
									 4.425473213195801,
									 52.171220800078025
								       },
								       {
									 4.424014091491699,
									 52.17119447992377
								       },
								       {
									 4.423928260803223,
									 52.16935203043088
								       },
								       {
									 4.422554969787598,
									 52.17095759783472
								       },
								       {
									 4.420838356018066,
									 52.170983918129124
								       },
								       {
									 4.419207572937012,
									 52.169299387895656
								       },
								       {
									 4.419121742248535,
									 52.1709049571992
								       },
								       {
									 4.418048858642578,
									 52.17095759783472
								       },
								       {
									 4.417877197265625,
									 52.169220423976036
								       },
								       {
									 4.416074752807617,
									 52.16935203043088
								       },
								       {
									 4.416203498840332,
									 52.170983918129124
								       },
								       {
									 4.415044784545898,
									 52.17101023840795}
								}});

		nlohmann::json request = {
			{"pilot_id", pilotID},
			{"start_time", now},
			{"end_time", end},
			{"buffer", 20},
			{"max_altitude_agl", 120},
			{"takeoff_latitude", latitude},
			{"takeoff_longitude", longitude},
			{"rulesets", {"custom_z5d341_drone_rules"}}/*,
			{"geometry", {
				 {"type", "Point"},
				 {"coordinates", {
					  longitude, latitude
				  }}
			 }}*/
		};
		request["geometry"] = geometry;

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
			//std::cout << "Briefing result:" << std::endl << j.dump(4) << std::endl;
			if (j["data"]["authorizations"].size() == 0) {
				m_authorization_status = "not required";
			} else {
				//				std::cout << "Authorization request: " << j["data"]["authorizations"][0]["status"] << std::endl;
				m_authorization_status = j["data"]["authorizations"][0]["status"];
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

