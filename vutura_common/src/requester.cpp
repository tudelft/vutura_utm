#include "requester.hpp"
#include <nng/protocol/reqrep0/req.h>


Requester::Requester(void* node, const std::string &url, void(*cb)(EventSource*)) :
	EventSource(node, -1, cb),
	_req_id(0)
{
	int rv;
	// start the Requester
	if ((rv = nng_req0_open(&_sock)) != 0) {
		fatal("[requester] nng_req0_open", rv);
	}

	if ((rv = nng_setopt_bool(_sock, NNG_OPT_TCP_KEEPALIVE, true)) != 0) {
		fatal("[requester] nng_setopt keepalive", rv);
	}

	if ((rv = nng_dialer_create(&_dialer, _sock, url.c_str())) != 0) {
		fatal("[subscription] dialer", rv);
	}

	// Set re-connect time to 500ms, helps when starting subscriber first
	if ((rv = nng_dialer_setopt_ms(_dialer, NNG_OPT_RECONNMINT, 500)) != 0) {
		fatal("[subscription] nng_dialer_setopt_ms", rv);
	}

	if ((rv = nng_dialer_setopt_ms(_dialer, NNG_OPT_RECONNMAXT, 0)) != 0) {
		fatal("[subscription] nng_dialer_setopt_ms", rv);
	}

	if ((rv = nng_dialer_start(_dialer, NNG_FLAG_NONBLOCK)) != 0) {
		fatal("[subscription] nng_dialer_start", rv);
	}


	//if ((rv = nng_dial(_sock, url.c_str(), NULL, NNG_FLAG_NONBLOCK)) != 0) {
	//	fatal("[requester] nng_dial", rv);
	//}

	if ((rv = nng_getopt_int(_sock, NNG_OPT_RECVFD, &_fd))) {
		fatal("[subscription] nng_getopt", rv);
	}
}

void Requester::send_request(const std::string &message)
{
	nng_msg *msg;
	nng_msg_alloc(&msg, 0);
	nng_msg_append(msg, message.c_str(), message.length());
	nng_sendmsg(_sock, msg, NNG_FLAG_NONBLOCK);
}

std::string Requester::get_reply()
{
	nng_msg *msg;
	nng_recvmsg(_sock, &msg, 0);
	std::string message((char*)nng_msg_body(msg), (char*)nng_msg_body(msg) + nng_msg_len(msg));

	// free the message
	nng_msg_free(msg);

	return message;
}

uint32_t Requester::req_id()
{
	return _req_id;
}
