#include "communicator.hpp"
#include <iostream>

Communicator::Communicator() {
	_headers = NULL;
	//_headers = curl_slist_append(m_headers, ("X-API-KEY: " + apiKey).c_str());
	curl_global_init(CURL_GLOBAL_DEFAULT);
}

unsigned int Communicator::post(const char *url, const char *postfields, std::string &response)
{
	return curl_post(url, _headers, postfields, response);
}

unsigned int Communicator::multipart_post(const char *url, const char *postfields, std::string &response)
{
	return curl_multipart_post(url, _headers, postfields, response);
}

unsigned int Communicator::get(const char *url, std::string &response) {
	return curl_get(url, _headers, response);
}

int Communicator::clear_headers()
{
	curl_slist_free_all(_headers);
	_headers = NULL;
	return 0;
}

int Communicator::add_header(std::string key, std::string value)
{
	std::string header = key + ": " + value;
	_headers = curl_slist_append(_headers, header.c_str());
}

unsigned int Communicator::curl_post(const char *url, const curl_slist *headers, const char *postfields, std::string &response) {
	CURLcode res = CURLE_FAILED_INIT;
	CURL *curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Communicator::writeFunction);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		std::cout << "Response: " << std::to_string(response_code) << std::endl;

		curl_easy_cleanup(curl);
	}

	return res;
}

unsigned int Communicator::curl_multipart_post(const char *url, const curl_slist *headers, const char *postfields, std::string &response)
{
	CURLcode res = CURLE_FAILED_INIT;
	CURL *curl = curl_easy_init();

	struct curl_httppost* post = NULL;
	struct curl_httppost* last = NULL;

	curl_formadd(&post, &last,
		     CURLFORM_COPYNAME, "meta",
		     CURLFORM_COPYCONTENTS, postfields,
		     CURLFORM_CONTENTTYPE, "application/json",
		     CURLFORM_END);

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Communicator::writeFunction);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}

		curl_easy_cleanup(curl);
	}
}



unsigned int Communicator::curl_get(const char *url, const curl_slist *headers, std::string &response) {
	CURLcode res = CURLE_FAILED_INIT;
	CURL *curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Communicator::writeFunction);
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

