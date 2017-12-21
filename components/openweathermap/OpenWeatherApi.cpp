/*
 * OpenWeatherApi.cpp
 *
 *  Created on: 19.12.2017
 *      Author: thomas
 */
#include "OpenWeatherApi.h"


static const char* tag = "OpenWeatherApi";


void OpenWeatherApi::init() {

}

void OpenWeatherApi::refresh() {
	ESP_LOGD(tag, "Weather Api Refresh");

	string request = "GET http://api.openweathermap.org/data/2.5/forecast?id=3302146&units=metric&lang=de_DE&appid=ea857f910cf094092c0f5cbb4ee5d2da HTTP/1.1\n"
		    "Host: api.openweathermap.org\n"
		    "Connection: close\n"
		    "User-Agent: esp-idf/1.0 esp32\n"
		    "\n";

	string response = client.getJson("api.openweathermap.org", request);

	apiObj = JSON::parseObject(response);

}
