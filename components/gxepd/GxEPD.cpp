/*
 * bme280.cpp
 *
 *  Created on: 11.12.2017
 *      Author: thomas
 */
#include "include/GxEPD.h"

static char tag[] = "GxEPD";

void GxEPD::init() {

	ESP_LOGD(tag, ">> GxEPD");
	ESP_LOGD(tag, ">> GxEPD %i", GxGDEW075T8_BUFFER_SIZE);

	//io.init(GPIO_NUM_23, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5);

	//fillScreen(GxEPD_WHITE);
	//start();
}

void GxEPD::fillScreen(uint16_t color) {
	/*uint8_t data = (color == GxEPD_BLACK) ? 0xFF : 0x00;
	for (uint16_t x = 0; x < sizeof(_buffer); x++)
	{
		_buffer[x] = data;
	}*/
}

void GxEPD::update() {
	ESP_LOGD(tag, ">> Update");
	/*io.transferByte(0x10);
	io.transfer(_buffer, GxGDEW075T8_BUFFER_SIZE);
	io.transferByte(0x12);*/
	ESP_LOGD(tag, ">> Update Finish");
}

void GxEPD::run(void *data) {

	while(true) {

		ESP_LOGD(tag, ">> GxEPD refresh");




		vTaskDelay(6000/portTICK_RATE_MS);

	}
}
