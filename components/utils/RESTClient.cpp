#include "RESTClient.h"

static const char* tag = "RestClient";

bool RESTClient::getJson(string web_server, string request) {

	const struct addrinfo hints = {
		.ai_flags = 0,
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};

	struct addrinfo *res;
	struct in_addr *addr;
	int s, r;
	char recv_buf[64];

	ESP_LOGD(tag, "Connected to AP");

	int err = getaddrinfo(web_server.c_str(), "80", &hints, &res);

	if(err != 0 || res == NULL) {
		ESP_LOGE(tag, "DNS lookup failed err=%d res=%p", err, res);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	ESP_LOGD(tag, "IP: %s", res->ai_canonname);

	addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
	ESP_LOGI(tag, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

	s = socket(res->ai_family, res->ai_socktype, 0);
	if(s < 0) {
		ESP_LOGE(tag, "... Failed to allocate socket.");
		freeaddrinfo(res);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	ESP_LOGI(tag, "... allocated socket");

	if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
		ESP_LOGE(tag, "... socket connect failed errno=%d", errno);
		close(s);
		freeaddrinfo(res);
		vTaskDelay(4000 / portTICK_PERIOD_MS);
	}

	ESP_LOGI(tag, "... connected");
	freeaddrinfo(res);

	if (write(s, request.c_str(), request.size()) < 0) {
		ESP_LOGE(tag, "... socket send failed");
		close(s);
		vTaskDelay(4000 / portTICK_PERIOD_MS);
	}
	ESP_LOGI(tag, "... socket send success");

	struct timeval receiving_timeout;
	receiving_timeout.tv_sec = 5;
	receiving_timeout.tv_usec = 0;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
			sizeof(receiving_timeout)) < 0) {
		ESP_LOGE(tag, "... failed to set socket receiving timeout");
		close(s);
		vTaskDelay(4000 / portTICK_PERIOD_MS);
	}
	ESP_LOGI(tag, "... set socket receiving timeout success");

	/* Read HTTP response */
	do {
		bzero(recv_buf, sizeof(recv_buf));
		r = read(s, recv_buf, sizeof(recv_buf)-1);
		for(int i = 0; i < r; i++) {
			putchar(recv_buf[i]);
		}
	} while(r > 0);

	ESP_LOGI(tag, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
	close(s);

	return true;
}
