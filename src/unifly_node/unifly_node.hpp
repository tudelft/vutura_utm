#pragma once
#include <string>
#include <curl/curl.h>

#include "vutura_common/communicator.hpp"

class UniflyNode {
public:
	UniflyNode();
	int start();

private:
	Communicator _comm;
};
