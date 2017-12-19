/*
 * bme280.h
 *
 *  Created on: 11.12.2017
 *      Author: thomas
 */
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "RESTClient.h"

class OpenWeatherApi {
private:
	RESTClient client;
public:
	void init();
	void refresh();

};
