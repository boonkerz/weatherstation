/*
 * weather.h
 *
 *  Created on: 16.12.2017
 *      Author: thomas
 */
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include "FileSystem.h"
#include "sdkconfig.h"
#include "Task.h"
#include "BME280.h"
#include "OpenWeatherApi.h"
#include "GxEPD.h"

#ifndef COMPONENTS_WEATHER_INCLUDE_WEATHER_H_
#define COMPONENTS_WEATHER_INCLUDE_WEATHER_H_


class Weather : public Task {
private:
	BME280 bme280Sensor;
	GxEPD epd;
	FileSystem vfs;
	OpenWeatherApi api;
	void displayBase();
	void display();
	void displayLocal();
	void displayApi();
	std::string timeStampToHReadble(long  timestamp);
public:
	void init();
	void run(void *data);

};


#endif /* COMPONENTS_WEATHER_INCLUDE_WEATHER_H_ */
