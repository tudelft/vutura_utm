#include "publisher.hpp"
#include <nng/protocol/pubsub0/pub.h>


Publisher::Publisher(std::string const& url)
{
	int rv;
	// start the publisher
	if ((rv = nng_pub0_open(&_sock)) != 0) {
		fatal("nng_pub0_open", rv);
	}

	if ((rv = nng_listen(_sock, url.c_str(), NULL, NNG_FLAG_NONBLOCK)) != 0) {
		fatal("nng_listen", rv);
	}

}

void Publisher::publish(const std::string &message)
{
	nng_msg *msg;
	nng_msg_alloc(&msg, 0);
	nng_msg_append(msg, message.c_str(), message.length());
	nng_sendmsg(_sock, msg, 0);
}
