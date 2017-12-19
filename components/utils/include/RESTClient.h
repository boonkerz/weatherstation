/*
 * RESTClient.h
 *
 *  Created on: Mar 12, 2017
 *      Author: kolban
 */
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
using namespace std;

class RESTClient {
private:
public:
	bool getJson(string web_server, string request);
};
