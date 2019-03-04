#include <string.h>
#include "listener_replier.hpp"
#include <nng/protocol/reqrep0/rep.h>

ListenerReplier::ListenerReplier(void *node, std::string url, void (*cb)(EventSource *)) :
	EventSource(node, -1, cb),
	_url(url)
{
	int rv;
	// Configure reply topic for utmsp request
	if ((rv = nng_rep0_open(&_socket)) != 0) {
		fatal("[listener-replier] nng_rep0_open", rv);
	}

	if ((rv = nng_listen(_socket, _url.c_str(), NULL, NNG_FLAG_NONBLOCK))) {
		fatal("[listener-replier] nng_listen", rv);
	}

	if ((rv = nng_getopt_int(_socket, NNG_OPT_RECVFD, &_fd))) {
		fatal("[listener-replier] nng_getopt", rv);
	}
}

ListenerReplier::~ListenerReplier()
{
	nng_close(_socket);
}

void ListenerReplier::send_response(std::string const& response)
{
	nng_msg *msg;
	nng_msg_alloc(&msg, response.length());
	memcpy(nng_msg_body(msg), response.c_str(), response.length());
	nng_sendmsg(_socket, msg, 0);
}

std::string ListenerReplier::get_message()
{
	nng_msg *msg;
	nng_recvmsg(_socket, &msg, 0);
	std::string message((char*)nng_msg_body(msg), (char*)nng_msg_body(msg) + nng_msg_len(msg));

	// free the message
	nng_msg_free(msg);

	return message;
}

