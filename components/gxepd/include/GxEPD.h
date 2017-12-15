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
#include <string.h>
#include "sdkconfig.h"
#include "Task.h"
#include "I2C.h"
#include "SPI.h"
#include "DisplayBase.h"

#ifndef COMPONENTS_GxEPD_INCLUDE_GxEPD_H_
#define COMPONENTS_GxEPD_INCLUDE_GxEPD_H_

#define EPD_BUSY_LEVEL 0

#define GxGDEW075T8_WIDTH 640
#define GxGDEW075T8_HEIGHT 384

#define GxGDEW075T8_PAGES 24

#define GxGDEW075T8_PAGE_HEIGHT (GxGDEW075T8_HEIGHT / GxGDEW075T8_PAGES)

#define BUSY_Pin GPIO_NUM_34

#define isEPD_BUSY  gpio_get_level(BUSY_Pin)

#define GxGDEW075T8_BUFFER_SIZE (uint32_t(GxGDEW075T8_WIDTH) * uint32_t(GxGDEW075T8_HEIGHT) / 8)

extern uint8_t tft_SmallFont[];
extern uint8_t tft_DefaultFont[];
extern uint8_t tft_Dejavu18[];
extern uint8_t tft_Dejavu24[];
extern uint8_t tft_Ubuntu16[];
extern uint8_t tft_Comic24[];
extern uint8_t tft_minya24[];
extern uint8_t tft_tooney32[];

class GxEPD : public DisplayBase {
private:
	SPI io;
	uint8_t drawBuff[GxGDEW075T8_BUFFER_SIZE];
	int _width = GxGDEW075T8_WIDTH;
	int _height = GxGDEW075T8_HEIGHT;
	void _send8pixel(uint8_t data);
	void _waitBusy();
	void _wakeUp();
	void _sleep();
	void _sendCommand(uint8_t value);

protected:

	void  _drawPixel(int x, int y, uint8_t val) override;
public:
	void init();
	void run(void *data);
	void update();
	void fillScreen(uint16_t color);
};

#endif /* COMPONENTS_GxEPD_INCLUDE_GxEPD_H_ */
