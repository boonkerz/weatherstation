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
#include "SPI.h"

#ifndef COMPONENTS_GxEPD_INCLUDE_GxEPD_H_
#define COMPONENTS_GxEPD_INCLUDE_GxEPD_H_

#define GxEPD_BLACK     0x0000
#define GxEPD_DARKGREY  0x7BEF      /* 128, 128, 128 */
#define GxEPD_LIGHTGREY 0xC618      /* 192, 192, 192 */
#define GxEPD_WHITE     0xFFFF
#define GxEPD_RED       0xF800      /* 255,   0,   0 */

#define GxGDEW075T8_WIDTH 640
#define GxGDEW075T8_HEIGHT 384

#define GxGDEW075T8_BUFFER_SIZE (uint32_t(GxGDEW075T8_WIDTH) * uint32_t(GxGDEW075T8_HEIGHT) / 8)

class GxEPD : public Task {
private:
	uint8_t _buffer[GxGDEW075T8_BUFFER_SIZE];
	//SPI io;
public:
	void init();
	void run(void *data);
	void fillScreen(uint16_t color);
	void update();
};

#endif /* COMPONENTS_GxEPD_INCLUDE_GxEPD_H_ */
