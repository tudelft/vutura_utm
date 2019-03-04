#include "subscription.hpp"
#include <nng/protocol/pubsub0/sub.h>

Subscription::Subscription(void *node, std::string url, void(*cb)(EventSource *)) :
	EventSource(node, -1, cb),
	_url(url)
{
	int rv;

	if ((rv = nng_sub0_open(&_socket)) != 0) {
		fatal("[subscription] nng_sub0_open", rv);
	}

	if ((rv = nng_setopt(_socket, NNG_OPT_SUB_SUBSCRIBE, "", 0)) != 0) {
		fatal("[subscription] nng_setopt", rv);
	}

	if ((rv = nng_dial(_socket, _url.c_str(), NULL, NNG_FLAG_NONBLOCK))) {
		fatal("[subscription] nng_dial", rv);
	}

	// Set re-connect time to 500ms, helps when starting subscriber first
	if ((rv = nng_setopt_ms(_socket, NNG_OPT_RECONNMINT, 500)) != 0) {
		fatal("[subscription] nng_setopt_ms", rv);
	}

	if ((rv = nng_setopt_ms(_socket, NNG_OPT_RECONNMAXT, 0)) != 0) {
		fatal("[subscription] nng_setopt_ms", rv);
	}

	if ((rv = nng_getopt_int(_socket, NNG_OPT_RECVFD, &_fd))) {
		fatal("[subscription] nng_getopt", rv);
	}
}

Subscription::~Subscription()
{
	nng_close(_socket);
}

std::string Subscription::get_message()
{
	nng_msg *msg;
	nng_recvmsg(_socket, &msg, 0);
	std::string message((char*)nng_msg_body(msg), (char*)nng_msg_body(msg) + nng_msg_len(msg));

	// free the message
	nng_msg_free(msg);

	return message;
}

