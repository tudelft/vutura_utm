#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>

#include <curl/curl.h>

#include "proto/messages.pb-c.h"

#include "airmap_config.h"

struct url_data {
    size_t size;
    char* data;
};

typedef enum airmap_state_t {
	AIRMAP_STATE_INIT = 0,
	AIRMAP_STATE_CONNECTED,
	AIRMAP_STATE_GOT_PILOT_ID,
	AIRMAP_STATE_FLIGHT_SUBMITTED,
	AIRMAP_STATE_FLIGHT_APPROVED,
	AIRMAP_STATE_FLIGHT_ACTIVE,
	AIRMAP_STATE_FLIGHT_COMPLETED
} airmap_state_t;

typedef struct airmap_node_t
{
	const char* name;
	airmap_state_t state;
	int req_fp_fd;
} airmap_node_t;

size_t write_data_callback(void *ptr, size_t size, size_t nmemb, struct url_data *data)
{
	printf("CALLBACK\n");
	size_t index = data->size;
	size_t n = (size * nmemb);
	char *tmp;

	data->size += n;
	tmp = realloc(data->data, data->size + 1); // +1 for \0

	if (tmp) {
		data->data = tmp;
	} else {
		if (data->data) {
			free(data->data);
		}
		fprintf(stderr, "failed realloc");
		return 0;
	}

	memcpy((data->data + index), ptr, n);
	data->data[data->size] = '\0';

	return n;
}

int airmap_connect(airmap_node_t *node)
{
	// Get the bearer-token
	CURL *curl = curl_easy_init();
	if (!curl) {
		printf("fail curl easy init");
		return 1;
	}

	struct url_data data;
	data.size = 0;
	data.data = malloc(4096);

	if (data.data == NULL) {
		fprintf(stderr, "malloc failed");
	}

	data.data[0] = '\0';

	char post_data[] = "grant_type=password"
			    "&client_id=" AIRMAP_CLIENT_ID
			    "&connection=Username-Password-Authentication"
			    "&username=" AIRMAP_USERNAME
			    "&password=" AIRMAP_PASSWORD
			    "&device=" AIRMAP_DEVICE_ID;

	CURLcode res;

	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, "https://" AIRMAP_SSO_HOST "/oauth/ro");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("return code: %i\n", res);
	}

	curl_easy_cleanup(curl);

	printf("Got:\n%s\n\n", data.data);

	free(data.data);

	return res;
}

int main(int argc, char **argv)
{
	airmap_node_t node = {
		.name = "airmap",
		.state = AIRMAP_STATE_INIT
	};

	// For now this is just for experimental api calls

	int rv;
	printf("Starting\n");

	airmap_connect(&node);

	exit(EXIT_SUCCESS);

	CURL *curl = curl_easy_init();
	if (!curl) {
		printf("fail");
		exit(EXIT_FAILURE);
	}

	char postfields[] = "grant_type=password"
			    "&client_id=" AIRMAP_CLIENT_ID
			    "&connection=Username-Password-Authentication"
			    "&username=" AIRMAP_USERNAME
			    "&password=" AIRMAP_PASSWORD
			    "&device=" AIRMAP_DEVICE_ID;

	CURLcode res;

	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, "https://" AIRMAP_SSO_HOST "/oauth/ro");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
	//curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("return code: %i\n", res);
	}

	curl_easy_cleanup(curl);
	exit(EXIT_SUCCESS);

	struct curl_slist *chunk = NULL;
	chunk = curl_slist_append(chunk, "X-API-Key: " AIRMAP_API_KEY);

	curl_easy_setopt(curl, CURLOPT_URL, "https://api.airmap.com/advisory/v1/weather?latitude=33.92&longitude=-118.45");
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
//	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		printf("Problem\n");
	}
	curl_easy_cleanup(curl);

}
