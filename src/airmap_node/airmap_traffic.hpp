#ifndef AIRMAP_TRAFFIC_HPP
#define AIRMAP_TRAFFIC_HPP

#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include "airmap_config.h"
#include "mqtt/async_client.h"
#include "nlohmann/json.hpp"
#include "vutura_common/vutura_common.pb.h"


const int	QOS = 1;
const int	N_RETRY_ATTEMPTS = 5;
const std::string CLIENT_ID("async_subcribe_cpp");


class AirmapTrafficActionListener : public virtual mqtt::iaction_listener
{
	std::string name_;

	void on_failure(const mqtt::token& tok) override {
		std::cout << name_ << " failure";
		if (tok.get_message_id() != 0)
			std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
		std::cout << std::endl;
	}

	void on_success(const mqtt::token& tok) override {
		std::cout << name_ << " success";
		if (tok.get_message_id() != 0)
			std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
		auto top = tok.get_topics();
		if (top && !top->empty())
			std::cout << "\ttoken topic: '" << (*top)[0] << "', ..." << std::endl;
		std::cout << std::endl;
	}

public:
	AirmapTrafficActionListener(const std::string& name) : name_(name) {}
};


class AirmapTrafficCallback : public virtual mqtt::callback,
		public virtual mqtt::iaction_listener
{
	std::string flightID_;
	nng_socket *pub_traffic_;

	std::string sa_topic = "uav/traffic/+/" + flightID_;
	//	std::string alert_topic = "uav/traffic/alert/" + flightID_;

	// Counter for the number of connection retries
	int nretry_;
	// The MQTT client
	mqtt::async_client& cli_;
	// Options to use if we need to reconnect
	mqtt::connect_options& connOpts_;
	// An action listener to display the result of actions.
	AirmapTrafficActionListener subListener_;

	// This deomonstrates manually reconnecting to the broker by calling
	// connect() again. This is a possibility for an application that keeps
	// a copy of it's original connect_options, or if the app wants to
	// reconnect with different options.
	// Another way this can be done manually, if using the same options, is
	// to just call the async_client::reconnect() method.
	void reconnect() {
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		try {
			cli_.connect(connOpts_, nullptr, *this);
		}
		catch (const mqtt::exception& exc) {
			std::cerr << "Error: " << exc.what() << std::endl;
			exit(1);
		}
	}

	// Re-connection failure
	void on_failure(const mqtt::token& tok) override {
		std::cout << "Connection attempt failed" << std::endl;
		//if (++nretry_ > N_RETRY_ATTEMPTS)
		//	exit(1);
		reconnect();
	}

	// (Re)connection success
	// Either this or connected() can be used for callbacks.
	void on_success(const mqtt::token& tok) override {}

	// (Re)connection success
	void connected(const std::string& cause) override {
		std::cout << "\nConnection success" << std::endl;
		std::cout << "\nSubscribing to topic '" << sa_topic << "'\n"
			  << "\tfor client " << airmap::client_id
			  << " using QoS" << QOS << "\n" << std::endl;

		cli_.subscribe(sa_topic, QOS, nullptr, subListener_);
	}

	// Callback for when the connection is lost.
	// This will initiate the attempt to manually reconnect.
	void connection_lost(const std::string& cause) override {
		std::cout << "\nConnection lost" << std::endl;
		if (!cause.empty())
			std::cout << "\tcause: " << cause << std::endl;

		std::cout << "Reconnecting..." << std::endl;
		nretry_ = 0;
		reconnect();
	}

	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override {
		//std::cout << "Message arrived" << std::endl;
		//std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
		//std::cout << "\tpayload: '" << msg->to_string() << "'\n" << std::endl;

		nlohmann::json j = nlohmann::json::parse(msg->to_string());
		nlohmann::json traffic;

		try {
			traffic = j["traffic"];

		} catch (...) {
			std::cerr << "does not contain traffic" << std::endl;
			return;
		}

		for (nlohmann::json::iterator it = traffic.begin(); it != traffic.end(); ++it) {
			try {
				if ((*it)["altitude"].get<std::string>() == "") {
					// invalid
					std::cout << "Invalid altitude" << std::endl;
					continue;
				}
				TrafficInfo tinfo;
				tinfo.set_unique_id((*it)["id"].get<std::string>());
				tinfo.set_aircraft_id((*it)["properties"]["aircraft_id"].get<std::string>());
				tinfo.set_timestamp((*it)["timestamp"].get<uint64_t>() / 1000);
				tinfo.set_recorded_time(std::stoull( (*it)["recorded_time"].get<std::string>() ));
				tinfo.set_lat(static_cast<int32_t>(std::stod( (*it)["latitude"].get<std::string>() ) * 1e7 ));
				tinfo.set_lon(static_cast<int32_t>(std::stod( (*it)["longitude"].get<std::string>() ) * 1e7 ));
				tinfo.set_alt(static_cast<int32_t>(std::stod( (*it)["altitude"].get<std::string>() ) * 1e3 ));

				// groundspeed and heading can be empty
				std::string groundspeed = (*it)["groundspeed"].get<std::string>();
				if (groundspeed != "") {
					tinfo.set_groundspeed(static_cast<uint32_t>(std::stod( groundspeed ) * 1e3));
				} else {
					tinfo.set_groundspeed(0);
				}

				std::string heading = (*it)["true_heading"].get<std::string>();
				if (heading != "") {
					tinfo.set_heading(std::stoul( heading ));
				} else {
					tinfo.set_heading(0);
				}

				std::string tinfo_data = tinfo.SerializeAsString();

				nng_msg *nngmsg;
				nng_msg_alloc(&nngmsg, tinfo_data.length());
				memcpy((char*)nng_msg_body(nngmsg), tinfo_data.c_str(), tinfo_data.length());
				nng_sendmsg(*pub_traffic_, nngmsg, 0);
				//std::cout << "Published traffic: " << tinfo.unique_id() << std::endl;

			} catch (...) {
				std::cerr << "json parse fail: " << (*it).dump(4) << std::endl;
				return;
			}
		}
		//std::cout << j["traffic"].dump(4) << std::endl;
	}

	void delivery_complete(mqtt::delivery_token_ptr token) override {}

public:
	AirmapTrafficCallback(mqtt::async_client& cli, mqtt::connect_options& connOpts, const std::string& flightID, nng_socket& pub_traffic)
		: nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription"), flightID_(flightID), pub_traffic_(&pub_traffic) {}
};


class AirmapTraffic
{
public:
	AirmapTraffic(int instance);
	~AirmapTraffic();

	int start(const std::string &flightID, const std::string &token);
	int stop();

private:
	mqtt::async_client _client;
	mqtt::connect_options _conn_opts;
	AirmapTrafficCallback* _cb;
	mqtt::ssl_options _sslopts;

	nng_socket _pub_traffic;
};

#endif // AIRMAP_TRAFFIC_HPP
