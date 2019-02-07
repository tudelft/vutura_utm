#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <poll.h>

#include <curl/curl.h>
#include <cjson/cJSON.h>

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
	char bearer_token[300];
	char pilot_id[100];
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
	int ret = 1;

	struct url_data data;
	data.size = 0;
	data.data = malloc(4096);

	if (data.data == NULL) {
		fprintf(stderr, "malloc failed");
		return ret;
	}
	data.data[0] = '\0';

	CURL *curl = curl_easy_init();
	if (!curl) {
		printf("fail curl easy init");
		free(data.data);
		return ret;
	}

	char post_data[] = "grant_type=password"
			    "&client_id=" AIRMAP_CLIENT_ID
			    "&connection=Username-Password-Authentication"
			    "&username=" AIRMAP_USERNAME
			    "&password=" AIRMAP_PASSWORD
			    "&scope=openid offline_access"
			    "&device=" AIRMAP_DEVICE_ID;

	CURLcode res;

	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_URL, "https://" AIRMAP_SSO_HOST "/oauth/ro");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "curl return code: %i\n", res);
		free(data.data);
		return ret;
	}

	curl_easy_cleanup(curl);

	// JSON parsing to get the token out
	cJSON *json = cJSON_Parse(data.data);

	if (json == NULL) {
		fprintf(stderr, "json parse failed");
		free(data.data);
		return ret;
	}

	cJSON *id_token = cJSON_GetObjectItemCaseSensitive(json, "id_token");
	if (cJSON_IsString(id_token) && id_token->valuestring != NULL) {
		size_t len = strlen(id_token->valuestring);
		printf("bearertokensize = %lu\n", sizeof(node->bearer_token));
		if ((len + 1) > sizeof(node->bearer_token)) {
			fprintf(stderr, "id_token too large for storage\n");
		} else {
			// storage is sufficiently large for the token
			memset(node->bearer_token, 0, sizeof(node->bearer_token));
			memcpy(node->bearer_token, id_token->valuestring, len);
			ret = 0;
		}
	}

	cJSON_Delete(json);

	free(data.data);

	return ret;
}

int airmap_get_pilot_id(airmap_node_t *node)
{
	int ret = 1;

	struct url_data data;
	data.size = 0;
	data.data = malloc(4096);

	if (data.data == NULL) {
		fprintf(stderr, "malloc failed");
		return ret;
	}
	data.data[0] = '\0';

	struct curl_slist *list = NULL;

	CURL *curl = curl_easy_init();
	if (!curl) {
		printf("fail curl easy init");
		free(data.data);
		return ret;
	}

	char token_header[350];
	char api_header[500];
	sprintf(token_header, "Authorization: Bearer %s", node->bearer_token);
	sprintf(api_header, "X-API-Key: %s", AIRMAP_API_KEY);
	printf("%s\n", token_header);
	printf("%s\n", api_header);

	list = curl_slist_append(list, token_header);
	list = curl_slist_append(list, api_header);

	CURLcode res;

	curl_easy_setopt(curl, CURLOPT_URL, "https://" AIRMAP_HOST "/pilot/v2/profile");
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

	res = curl_easy_perform(curl);
	curl_slist_free_all(list);

	if (res != CURLE_OK) {
		fprintf(stderr, "curl return code: %i\n", res);
		free(data.data);
		return ret;
	}

	curl_easy_cleanup(curl);

	// JSON parsing to get the token out
	cJSON *json = cJSON_Parse(data.data);

	if (json == NULL) {
		fprintf(stderr, "json parse failed");
		free(data.data);
		return ret;
	}

	cJSON *user_data = cJSON_GetObjectItemCaseSensitive(json, "data");
	if (cJSON_IsObject(user_data)) {
		cJSON *pilot_id = cJSON_GetObjectItemCaseSensitive(user_data, "id");
		if (cJSON_IsString(pilot_id) && pilot_id->valuestring != NULL) {
			printf("pilot_id: %s\n", pilot_id->valuestring);
			ssize_t len = strlen(pilot_id->valuestring);
			if ((len + 1) > sizeof(node->pilot_id)) {
				fprintf(stderr, "pilot_id too large for storage\n");
			} else {
				// copy pilot_id
				memset(node->pilot_id, 0, sizeof(node->pilot_id));
				memcpy(node->pilot_id, pilot_id->valuestring, len);
				ret = 0;
			}
		}
	}

	cJSON_Delete(json);

	free(data.data);

	return ret;
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

	if ((rv = airmap_connect(&node)) == 0) {
		node.state = AIRMAP_STATE_CONNECTED;

	} else {
		fprintf(stderr, "airmap_connect failed\n");
		exit(EXIT_FAILURE);
	}

	if ((rv = airmap_get_pilot_id(&node)) == 0) {
		node.state = AIRMAP_STATE_GOT_PILOT_ID;

	} else {
		fprintf(stderr, "airmap_get_pilot_id failed\n");
		exit(EXIT_FAILURE);
	}

	if (node.state == AIRMAP_STATE_GOT_PILOT_ID) {
		printf("\n\nGOT PILOT ID: %s\n", node.pilot_id);
	}

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
