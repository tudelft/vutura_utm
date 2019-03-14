#include <curl/curl.h>
#include "unifly_config.h"
#include "unifly_node.hpp"


UniflyNode::UniflyNode()
{

}

int UniflyNode::start()
{
	std::string url = "https://" UNIFLY_SSO_HOST "/oauth/token";
	_comm.clear_headers();
	_comm.add_header("Authorization", "Basic " UNIFLY_CLIENT_ID);
	_comm.add_header("Content-Type", "multipart/form-data");

}
