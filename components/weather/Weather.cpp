/*
 * weather.cpp
 *
 *  Created on: 16.12.2017
 *      Author: thomas
 */

#include "include/Weather.h"


static char tag[] = "WatherThread";

void Weather::init() {
	bme280Sensor.init();
	epd.init();
	api.init();
	start();
}

void Weather::run(void *data) {

	while(true) {
		ESP_LOGD(tag, ">> refresh");

		bme280Sensor.streamSensorDataNormalMode();
		api.refresh();

		display();

		vTaskDelay(10000/portTICK_RATE_MS);
	}
}



void Weather::display() {
	displayBase();

	epd.update();
}


void Weather::displayBase() {
	epd.clean();

	epd.setFont(6, NULL);
	epd.drawText((char *)"Wetter Station", 0, 0);

	epd.setFont(2, NULL);

	char buffer [50];
	sprintf(buffer, "Temperatur: %0.2f C", bme280Sensor.comp_data.temperature);

	epd.drawText(buffer, 0, 40);

	sprintf(buffer, "Luftfeuchte: %0.2f %%", bme280Sensor.comp_data.humidity);

	epd.drawText(buffer, 0, 65);

	sprintf(buffer, "Luftdruck: %0.2f hPa", bme280Sensor.comp_data.pressure/100);

	epd.drawText(buffer, 0, 90);

	epd.drawFastHLine(0, 120, epd.getWidth(), EPD_BLACK);
}
