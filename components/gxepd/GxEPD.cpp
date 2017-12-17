/*
 * bme280.cpp
 *
 *  Created on: 11.12.2017
 *      Author: thomas
 */
#include "include/GxEPD.h"

static char tag[] = "GxEPD";



void GxEPD::init() {

	dispWin = {
	  .x1 = 0,
	  .y1 = 0,
	  .x2 = GxGDEW075T8_WIDTH-1,
	  .y2 = GxGDEW075T8_HEIGHT-1,
	};

	ESP_LOGD(tag, ">> GxEPD");
	ESP_LOGD(tag, ">> GxEPD %i", GxGDEW075T8_BUFFER_SIZE);

	gpio_set_direction(BUSY_Pin, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BUSY_Pin, GPIO_PULLUP_ONLY);
	gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
	gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);

	io.init(GPIO_NUM_23, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5);

}

int GxEPD::getWidth() {
	return _width;
}

int GxEPD::getHeight() {
	return _height;
}

void GxEPD::clean() {
	fillScreen(EPD_WHITE);
}


void GxEPD::update() {
	_wakeUp();
	_sendCommand(0x10);

	for (uint32_t i = 0; i < GxGDEW075T8_BUFFER_SIZE; i++)
	{
		_send8pixel(i < sizeof(drawBuff) ? drawBuff[i] : 0x00);
	}
	_sendCommand(0x12);
	_waitBusy();
	_sleep();
}

void GxEPD::_send8pixel(uint8_t data)
{
  for (uint8_t j = 0; j < 8; j++)
  {
    uint8_t t = data & 0x80 ? 0x00 : 0x03;
    t <<= 4;
    data <<= 1;
    j++;
    t |= data & 0x80 ? 0x00 : 0x03;
    data <<= 1;
    io.transferByte(t);
  }
}

void GxEPD::fillScreen(uint16_t color) {
	uint8_t data = (color == EPD_BLACK) ? 0xFF : 0x00;
	for (uint16_t x = 0; x < sizeof(drawBuff); x++)
	{
		drawBuff[x] = data;
	}
}

void GxEPD::_wakeUp()
{
    /**********************************release flash sleep**********************************/
  _sendCommand(0X65);     //FLASH CONTROL
  io.transferByte(0x01);

  _sendCommand(0xAB);

  _sendCommand(0X65);     //FLASH CONTROL
  io.transferByte(0x00);
  /**********************************release flash sleep**********************************/
  _sendCommand(0x01);
  io.transferByte (0x37);       //POWER SETTING
  io.transferByte (0x00);

  _sendCommand(0X00);     //PANNEL SETTING
  io.transferByte(0xCF);
  io.transferByte(0x08);

  _sendCommand(0x06);     //boost
  io.transferByte (0xc7);
  io.transferByte (0xcc);
  io.transferByte (0x28);

  _sendCommand(0x30);     //PLL setting
  io.transferByte (0x3c);

  _sendCommand(0X41);     //TEMPERATURE SETTING
  io.transferByte(0x00);

  _sendCommand(0X50);     //VCOM AND DATA INTERVAL SETTING
  io.transferByte(0x77);

  _sendCommand(0X60);     //TCON SETTING
  io.transferByte(0x22);

  _sendCommand(0x61);     //tres 640*384
  io.transferByte (0x02);       //source 640
  io.transferByte (0x80);
  io.transferByte (0x01);       //gate 384
  io.transferByte (0x80);

  _sendCommand(0X82);     //VDCS SETTING
  io.transferByte(0x1E);        //decide by LUT file

  _sendCommand(0xe5);     //FLASH MODE
  io.transferByte(0x03);

  _sendCommand(0x04);     //POWER ON
  _waitBusy();
}

void GxEPD::_sleep()
{
	_sendCommand(0X65);     //FLASH CONTROL
	io.transferByte(0x01);

	_sendCommand(0xB9);

	_sendCommand(0X65);     //FLASH CONTROL
	io.transferByte(0x00);

	_sendCommand(0x02);     // POWER OFF
	_waitBusy();
}

void GxEPD::_sendCommand(uint8_t value) {
	gpio_set_level(GPIO_NUM_17, 0);
	gpio_set_level(GPIO_NUM_5, 0);
	io.transferByte(value);
	gpio_set_level(GPIO_NUM_5, 1);
	gpio_set_level(GPIO_NUM_17, 1);
}

void GxEPD::_waitBusy()
{
	while(isEPD_BUSY == EPD_BUSY_LEVEL) {
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

void GxEPD::_drawPixel(int x, int y, uint8_t val)
{
	if (orientation == LANDSCAPE_180) {
		x = _width - x - 1;
		y = _height - y - 1;
	}
	if (_gs) {
		val &= 0x0F;
		//if (gs_drawBuff[(y * _width) + x] != val) {
			gs_drawBuff[(y * _width) + x] = val;
			gs_used_shades |= (1<<val);
		//}
	}
	else {
	  uint16_t i = x / 8 + y * GxGDEW075T8_WIDTH / 8;
	  if (_current_page < 1)
	  {
		if (i >= sizeof(drawBuff)) return;
	  }
	  else
	  {
		y -= _current_page * GxGDEW075T8_HEIGHT;
		if ((y < 0) || (y >= GxGDEW075T8_HEIGHT)) return;
		i = x / 8 + y * GxGDEW075T8_WIDTH / 8;
	  }
	  if (val)
		  drawBuff[i] = (drawBuff[i] | (1 << (7 - x % 8)));
	  else
		  drawBuff[i] = (drawBuff[i] & (0xFF ^ (1 << (7 - x % 8))));
	}
}

