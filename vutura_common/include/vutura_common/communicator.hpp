#pragma once
#include <string>
#include <curl/curl.h>

// Communicator
class Communicator
{
public:
	Communicator();

	unsigned int post(const char* url, const char* postfields, std::string& response);
	unsigned int get(const char* url, std::string& response);
	int clear_headers();
	int add_header(std::string key, std::string value);

private:
	static size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
		data->append((char*)ptr, size * nmemb);
		return size * nmemb;
	}

	unsigned int curl_post(const char* url, const struct curl_slist* headers, const char* postfields, std::string& response);
	unsigned int curl_get(const char* url, const struct curl_slist* headers, std::string& response);

	struct curl_slist *_headers;

};
