/*
 * bme280.h
 *
 *  Created on: 11.12.2017
 *      Author: thomas
 */
#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "Task.h"
#include "I2C.h"

#ifndef COMPONENTS_BME280_INCLUDE_BME280_H_
#define COMPONENTS_BME280_INCLUDE_BME280_H_

#define BME280_ADDRESS 0x77;

#define BME280_CHIP_ID  UINT8_C(0x60)

#define BME280_CHIP_ID_ADDR					UINT8_C(0xD0)


class BME280 : public Task {
private:
	I2C i2cBus;
public:
	void init();
	void run(void *data);
};

#endif /* COMPONENTS_BME280_INCLUDE_BME280_H_ */
