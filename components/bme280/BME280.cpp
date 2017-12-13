/*
 * bme280.cpp
 *
 *  Created on: 11.12.2017
 *      Author: thomas
 */
#include "include/BME280.h"

static char tag[] = "BME280";

void BME280::init() {

	i2cBus.init(0x77, GPIO_NUM_21, GPIO_NUM_22, I2C::DEFAULT_CLK_SPEED, I2C_NUM_0);

	ESP_LOGD(tag, ">> BME280Sensor");
	//start();
}

void BME280::run(void *data) {

	while(true) {

		ESP_LOGD(tag, ">> BME280Sensor refresh");

		uint8_t chip_id = 0;
		int8_t rslt;

		i2cBus.setAddress(0x77);
		i2cBus.beginTransaction();
		i2cBus.write(BME280_CHIP_ID_ADDR, true);
		i2cBus.start();
		i2cBus.read(&chip_id, false);
		i2cBus.endTransaction();

		ESP_LOGD(tag, ">> BME280Sensor read %d", chip_id);

		vTaskDelay(6000/portTICK_RATE_MS);

	}
}
