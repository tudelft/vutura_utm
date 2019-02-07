#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>

#include <curl/curl.h>

#include "proto/messages.pb-c.h"

#include "airmap_config.h"

typedef struct airmap_node_t
{
	const char* name;
	int req_fp_fd;
} airmap_node_t;

int main(int argc, char **argv)
{
	airmap_node_t node = {
		.name = "airmap"
	};

	// For now this is just for experimental api calls

	int rv;
	printf("Starting\n");

	CURL *curl = curl_easy_init();
	if (!curl) {
		exit(EXIT_FAILURE);
	}

	CURLcode res;

	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, "https://" AIRMAP_SSO_HOST "/oauth/ro");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "grant_type=password&client_id=123");
	//curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("return code: %i\n", res);
	}

	curl_easy_cleanup(curl);
	exit(EXIT_SUCCESS);

	struct curl_slist *chunk = NULL;
	chunk = curl_slist_append(chunk, "X-API-Key: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJjcmVkZW50aWFsX2lkIjoiY3JlZGVudGlhbHxlOWtQNE9nSEpBWjJ6cFNnTFpCb0doZUVNTTRRIiwiYXBwbGljYXRpb25faWQiOiJhcHBsaWNhdGlvbnx6d2s1UUU1SE5ZVzVMWHVkOVBvR2xIbzlLWVhvIiwib3JnYW5pemF0aW9uX2lkIjoiZGV2ZWxvcGVyfHo5eGVSZExVTG80YjhRaTM4NHZRV1N6MzB3bDgiLCJpYXQiOjE1NDk0NzEwMjl9.wHelYLl7EmRCD7zZ3YFkXVvoeOVVXU_D7Vtsx5U_T6U");

	curl_easy_setopt(curl, CURLOPT_URL, "https://api.airmap.com/advisory/v1/weather?latitude=33.92&longitude=-118.45");
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
//	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("Problem\n");
	}
	curl_easy_cleanup(curl);

}
