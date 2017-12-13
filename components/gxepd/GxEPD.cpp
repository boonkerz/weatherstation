/*
 * bme280.cpp
 *
 *  Created on: 11.12.2017
 *      Author: thomas
 */
#include "include/GxEPD.h"

static char tag[] = "GxEPD";

extern uint8_t tft_SmallFont[];
extern uint8_t tft_DefaultFont[];
extern uint8_t tft_Dejavu18[];
extern uint8_t tft_Dejavu24[];
extern uint8_t tft_Ubuntu16[];
extern uint8_t tft_Comic24[];
extern uint8_t tft_minya24[];
extern uint8_t tft_tooney32[];


void GxEPD::init() {

	dispWin = {
	  .x1 = 0,
	  .y1 = 0,
	  .x2 = GxGDEW075T8_WIDTH-1,
	  .y2 = GxGDEW075T8_HEIGHT-1,
	};

	cfont = {
		.font = tft_DefaultFont,
		.x_size = 10,
		.y_size = 0x0B,
		.offset = 0,
		.numchars = 95,
		.size = 20,
		.max_x_size = 95,
		.bitmap = 1,
		.color = 15
	};

	ESP_LOGD(tag, ">> GxEPD");
	ESP_LOGD(tag, ">> GxEPD %i", GxGDEW075T8_BUFFER_SIZE);

	_current_page = -1;

	gpio_set_direction(BUSY_Pin, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BUSY_Pin, GPIO_PULLUP_ONLY);
	gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
	gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);

	io.init(GPIO_NUM_23, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5);
	fillScreen(GxEPD_WHITE);
	//start();
	//fillRect(100,100,200,200, EPD_BLACK);
	//fillRect(140,140,20,20, EPD_WHITE);
	setFont(7, NULL);
	drawText((char *)"Welcome to This ", 4, 20);
	update();
}

int GxEPD::getFontHeight()
{
  if (cfont.bitmap == 1) return cfont.y_size;			// Bitmap font
  else if (cfont.bitmap == 2) return _7seg_height();	// 7-segment font
  return 0;
}

void GxEPD::fillScreen(uint16_t color) {
	uint8_t data = (color == GxEPD_BLACK) ? 0xFF : 0x00;
	for (uint16_t x = 0; x < sizeof(drawBuff); x++)
	{
		drawBuff[x] = data;
	}
}

void GxEPD::setFont(uint8_t font, const char *font_file)
{
  cfont.font = NULL;

  if (font == FONT_7SEG) {
    cfont.bitmap = 2;
    cfont.x_size = 24;
    cfont.y_size = 6;
    cfont.offset = 0;
    cfont.color  = _fg;
  }
  else {
	  if (font == DEJAVU18_FONT) cfont.font = tft_Dejavu18;
	  else if (font == DEJAVU24_FONT) cfont.font = tft_Dejavu24;
	  else if (font == UBUNTU16_FONT) cfont.font = tft_Ubuntu16;
	  else if (font == COMIC24_FONT) cfont.font = tft_Comic24;
	  else if (font == MINYA24_FONT) cfont.font = tft_minya24;
	  else if (font == TOONEY32_FONT) cfont.font = tft_tooney32;
	  else if (font == SMALL_FONT) cfont.font = tft_SmallFont;
	  else cfont.font = tft_DefaultFont;

	  cfont.bitmap = 1;
	  cfont.x_size = cfont.font[0];
	  cfont.y_size = cfont.font[1];
	  if (cfont.x_size > 0) {
		  cfont.offset = cfont.font[2];
		  cfont.numchars = cfont.font[3];
		  cfont.size = cfont.x_size * cfont.y_size * cfont.numchars;
	  }
	  else {
		  cfont.offset = 4;
		  _getMaxWidthHeight();
	  }
	  //_testFont();
  }
}

void GxEPD::_getMaxWidthHeight()
{
	uint16_t tempPtr = 4; // point at first char data
	uint8_t cc, cw, ch, cd, cy;

	cfont.numchars = 0;
	cfont.max_x_size = 0;

    cc = cfont.font[tempPtr++];
    while (cc != 0xFF)  {
    	cfont.numchars++;
        cy = cfont.font[tempPtr++];
        cw = cfont.font[tempPtr++];
        ch = cfont.font[tempPtr++];
        tempPtr++;
        cd = cfont.font[tempPtr++];
        cy += ch;
		if (cw > cfont.max_x_size) cfont.max_x_size = cw;
		if (cd > cfont.max_x_size) cfont.max_x_size = cd;
		if (ch > cfont.y_size) cfont.y_size = ch;
		if (cy > cfont.y_size) cfont.y_size = cy;
		if (cw != 0) {
			// packed bits
			tempPtr += (((cw * ch)-1) / 8) + 1;
		}
	    cc = cfont.font[tempPtr++];
	}
    cfont.size = tempPtr;
}

void GxEPD::update() {
	ESP_LOGD(tag, ">> Update");

	_wakeUp();
	_sendCommand(0x10);

	for (uint32_t i = 0; i < GxGDEW075T8_BUFFER_SIZE; i++)
	{
		_send8pixel(i < sizeof(drawBuff) ? drawBuff[i] : 0x00);
	}
	_sendCommand(0x12);
	ESP_LOGD(tag, ">> Update Finish");

	_waitBusy();

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

void GxEPD::run(void *data) {

	while(true) {

		ESP_LOGD(tag, ">> GxEPD refresh");

		vTaskDelay(6000/portTICK_RATE_MS);

	}
}

void GxEPD::drawText(char *st, int x, int y) {
	int stl, i, tmpw, tmph, fh;
	uint8_t ch;

	if (cfont.bitmap == 0) return; // wrong font selected

	// ** Rotated strings cannot be aligned
	if ((font_rotate != 0) && ((x <= CENTER) || (y <= CENTER))) return;

	if ((x < LASTX) || (font_rotate == 0)) EPD_OFFSET = 0;

	if ((x >= LASTX) && (x < LASTY)) x = EPD_X + (x-LASTX);
	else if (x > CENTER) x += dispWin.x1;

	if (y >= LASTY) y = EPD_Y + (y-LASTY);
	else if (y > CENTER) y += dispWin.y1;

	// ** Get number of characters in string to print
	stl = strlen(st);

	// ** Calculate CENTER, RIGHT or BOTTOM position
	tmpw = _getStringWidth(st);	// string width in pixels
	fh = cfont.y_size;			// font height
	if ((cfont.x_size != 0) && (cfont.bitmap == 2)) {
		// 7-segment font
		fh = (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size);  // 7-seg character height
	}

	if (x == RIGHT) x = dispWin.x2 - tmpw + dispWin.x1;
	else if (x == CENTER) x = (((dispWin.x2 - dispWin.x1 + 1) - tmpw) / 2) + dispWin.x1;

	if (y == BOTTOM) y = dispWin.y2 - fh + dispWin.y1;
	else if (y==CENTER) y = (((dispWin.y2 - dispWin.y1 + 1) - (fh/2)) / 2) + dispWin.y1;

	if (x < dispWin.x1) x = dispWin.x1;
	if (y < dispWin.y1) y = dispWin.y1;
	if ((x > dispWin.x2) || (y > dispWin.y2)) return;

	 EPD_X = x;
	 EPD_Y = y;

	// ** Adjust y position
	tmph = cfont.y_size; // font height
	// for non-proportional fonts, char width is the same for all chars
	tmpw = cfont.x_size;
	if (cfont.x_size != 0) {
		if (cfont.bitmap == 2) {	// 7-segment font
			tmpw = _7seg_width();	// character width
			tmph = _7seg_height();	// character height
		}
	}
	else EPD_OFFSET = 0;	// fixed font; offset not needed

	if (( EPD_Y + tmph - 1) > dispWin.y2) return;

	int offset = EPD_OFFSET;

	for (i=0; i<stl; i++) {
		ch = st[i]; // get string character

		if (ch == 0x0D) { // === '\r', erase to eol ====
			if ((!font_transparent) && (font_rotate==0)) _fillRect( EPD_X, EPD_Y,  dispWin.x2+1- EPD_X, tmph, _bg);
		}

		else if (ch == 0x0A) { // ==== '\n', new line ====
			if (cfont.bitmap == 1) {
				 EPD_Y += tmph + font_line_space;
				if ( EPD_Y > (dispWin.y2-tmph)) break;
				 EPD_X = dispWin.x1;
			}
		}

		else { // ==== other characters ====
			if (cfont.x_size == 0) {
				// for proportional font get character data to 'fontChar'
				if (getCharPtr(ch)) tmpw = fontChar.xDelta;
				else continue;
			}

			// check if character can be displayed in the current line
			if (( EPD_X+tmpw) > (dispWin.x2)) {
				if (text_wrap == 0) break;
				 EPD_Y += tmph + font_line_space;
				if ( EPD_Y > (dispWin.y2-tmph)) break;
				 EPD_X = dispWin.x1;
			}

			// Let's print the character
			if (cfont.x_size == 0) {
				// == proportional font
				if (font_rotate == 0) EPD_X += _printProportionalChar( EPD_X, EPD_Y) + 1;
				else {
					// rotated proportional font
					offset += _rotatePropChar(x, y, offset);
					EPD_OFFSET = offset;
				}
			}
			else {
				if (cfont.bitmap == 1) {
					// == fixed font
					if ((ch < cfont.offset) || ((ch-cfont.offset) > cfont.numchars)) ch = cfont.offset;
					if (font_rotate == 0) {
						_printChar(ch, EPD_X, EPD_Y);
						 EPD_X += tmpw;
					}
					else _rotateChar(ch, x, y, i);
				}
				else if (cfont.bitmap == 2) {
					// == 7-segment font ==
					_draw7seg( EPD_X, EPD_Y, ch, cfont.y_size, cfont.x_size, _fg);
					 EPD_X += (tmpw + 2);
				}
			}
		}
	}
}

static const uint16_t font_bcd[] = {
  0x200, // 0010 0000 0000  // -
  0x080, // 0000 1000 0000  // .
  0x06C, // 0100 0110 1100  // /, degree
  0x05f, // 0000 0101 1111, // 0
  0x005, // 0000 0000 0101, // 1
  0x076, // 0000 0111 0110, // 2
  0x075, // 0000 0111 0101, // 3
  0x02d, // 0000 0010 1101, // 4
  0x079, // 0000 0111 1001, // 5
  0x07b, // 0000 0111 1011, // 6
  0x045, // 0000 0100 0101, // 7
  0x07f, // 0000 0111 1111, // 8
  0x07d, // 0000 0111 1101  // 9
  0x900  // 1001 0000 0000  // :
};



//============================================================================
void GxEPD::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color) {
	ESP_LOGD(tag, ">> fillRect begin");
	_fillRect(x+dispWin.x1, y+dispWin.y1, w, h, color);
	ESP_LOGD(tag, ">> fillRect end");
}

//======================================================================
void GxEPD::drawFastVLine(int16_t x, int16_t y, int16_t h, color_t color) {
	_drawFastVLine(x+dispWin.x1, y+dispWin.y1, h, color);
}

//======================================================================
void GxEPD::drawFastHLine(int16_t x, int16_t y, int16_t w, color_t color) {
	_drawFastHLine(x+dispWin.x1, y+dispWin.y1, w, color);
}

void GxEPD::_draw7seg(int16_t x, int16_t y, int8_t num, int16_t w, int16_t l, color_t color) {
  /* TODO: clipping */
  if (num < 0x2D || num > 0x3A) return;

  int16_t c = font_bcd[num-0x2D];
  int16_t d = 2*w+l+1;

  // === Clear unused segments ===
  /*
  if (!(c & 0x001)) barVert(x+d, y+d, w, l, _bg, _bg);
  if (!(c & 0x002)) barVert(x,   y+d, w, l, _bg, _bg);
  if (!(c & 0x004)) barVert(x+d, y, w, l, _bg, _bg);
  if (!(c & 0x008)) barVert(x,   y, w, l, _bg, _bg);
  if (!(c & 0x010)) barHor(x, y+2*d, w, l, _bg, _bg);
  if (!(c & 0x020)) barHor(x, y+d, w, l, _bg, _bg);
  if (!(c & 0x040)) barHor(x, y, w, l, _bg, _bg);

  if (!(c & 0x080)) {
    // low point
    _fillRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, _bg);
    if (cfont.offset) _drawRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, _bg);
  }
  if (!(c & 0x100)) {
    // down middle point
    _fillRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, _bg);
    if (cfont.offset) _drawRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, _bg);
  }
  if (!(c & 0x800)) {
	// up middle point
    _fillRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, _bg);
    if (cfont.offset) _drawRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, _bg);
  }
  if (!(c & 0x200)) {
    // middle, minus
    _fillRect(x+2*w+1, y+d, l, 2*w+1, _bg);
    if (cfont.offset) _drawRect(x+2*w+1, y+d, l, 2*w+1, _bg);
  }
  */
  _barVert(x+d, y+d, w, l, _bg, _bg);
  _barVert(x,   y+d, w, l, _bg, _bg);
  _barVert(x+d, y, w, l, _bg, _bg);
  _barVert(x,   y, w, l, _bg, _bg);
  _barHor(x, y+2*d, w, l, _bg, _bg);
  _barHor(x, y+d, w, l, _bg, _bg);
  _barHor(x, y, w, l, _bg, _bg);

  _fillRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, _bg);
  _drawRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, _bg);
  _fillRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, _bg);
  _drawRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, _bg);
  _fillRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, _bg);
  _drawRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, _bg);
  _fillRect(x+2*w+1, y+d, l, 2*w+1, _bg);
  _drawRect(x+2*w+1, y+d, l, 2*w+1, _bg);

  // === Draw used segments ===
  if (c & 0x001) _barVert(x+d, y+d, w, l, color, cfont.color);	// down right
  if (c & 0x002) _barVert(x,   y+d, w, l, color, cfont.color);	// down left
  if (c & 0x004) _barVert(x+d, y, w, l, color, cfont.color);		// up right
  if (c & 0x008) _barVert(x,   y, w, l, color, cfont.color);		// up left
  if (c & 0x010) _barHor(x, y+2*d, w, l, color, cfont.color);	// down
  if (c & 0x020) _barHor(x, y+d, w, l, color, cfont.color);		// middle
  if (c & 0x040) _barHor(x, y, w, l, color, cfont.color);		// up

  if (c & 0x080) {
    // low point
    _fillRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, color);
    if (cfont.offset) _drawRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, cfont.color);
  }
  if (c & 0x100) {
    // down middle point
    _fillRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, color);
    if (cfont.offset) _drawRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, cfont.color);
  }
  if (c & 0x800) {
	// up middle point
    _fillRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, color);
    if (cfont.offset) _drawRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, cfont.color);
  }
  if (c & 0x200) {
    // middle, minus
    _fillRect(x+2*w+1, y+d, l, 2*w+1, color);
    if (cfont.offset) _drawRect(x+2*w+1, y+d, l, 2*w+1, cfont.color);
  }
}

int GxEPD::_getStringWidth(char* str)
{
    int strWidth = 0;

	if (cfont.bitmap == 2) strWidth = ((_7seg_width()+2) * strlen(str)) - 2;	// 7-segment font
	else if (cfont.x_size != 0) strWidth = strlen(str) * cfont.x_size;			// fixed width font
	else {
		// calculate the width of the string of proportional characters
		char* tempStrptr = str;
		while (*tempStrptr != 0) {
			if (getCharPtr(*tempStrptr++)) {
				strWidth += (((fontChar.width > fontChar.xDelta) ? fontChar.width : fontChar.xDelta) + 1);
			}
		}
		strWidth--;
	}
	return strWidth;
}

int GxEPD::_7seg_width()
{
	return (2 * (2 * cfont.y_size + 1)) + cfont.x_size;
}

//-----------------------
int GxEPD::_7seg_height()
{
	return (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size);
}

uint8_t GxEPD::getCharPtr(uint8_t c) {
  uint16_t tempPtr = 4; // point at first char data

  do {
	fontChar.charCode = cfont.font[tempPtr++];
    if (fontChar.charCode == 0xFF) return 0;

    fontChar.adjYOffset = cfont.font[tempPtr++];
    fontChar.width = cfont.font[tempPtr++];
    fontChar.height = cfont.font[tempPtr++];
    fontChar.xOffset = cfont.font[tempPtr++];
    fontChar.xOffset = fontChar.xOffset < 0x80 ? fontChar.xOffset : -(0xFF - fontChar.xOffset);
    fontChar.xDelta = cfont.font[tempPtr++];

    if (c != fontChar.charCode && fontChar.charCode != 0xFF) {
      if (fontChar.width != 0) {
        // packed bits
        tempPtr += (((fontChar.width * fontChar.height)-1) / 8) + 1;
      }
    }
  } while ((c != fontChar.charCode) && (fontChar.charCode != 0xFF));

  fontChar.dataPtr = tempPtr;
  if (c == fontChar.charCode) {
    if (font_forceFixed > 0) {
      // fix width & offset for forced fixed width
      fontChar.xDelta = cfont.max_x_size;
      fontChar.xOffset = (fontChar.xDelta - fontChar.width) / 2;
    }
  }
  else return 0;

  return 1;
}

void GxEPD::_drawPixel(int x, int y, uint8_t val)
{
	ESP_LOGD(tag, ">> drawPixel BEFORE %d %d %d ", x, y, val);
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
		y -= _current_page * GxGDEW075T8_PAGE_HEIGHT;
		if ((y < 0) || (y >= GxGDEW075T8_PAGE_HEIGHT)) return;
		i = x / 8 + y * GxGDEW075T8_WIDTH / 8;
	  }

	  if (val)
		  drawBuff[i] = (drawBuff[i] | (1 << (7 - x % 8)));
	  else
		  drawBuff[i] = (drawBuff[i] & (0xFF ^ (1 << (7 - x % 8))));


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
	/*IO.writeCommandTransaction(0X65);     //FLASH CONTROL
	IO.writeDataTransaction(0x01);

	IO.writeCommandTransaction(0xB9);

	IO.writeCommandTransaction(0X65);     //FLASH CONTROL
	IO.writeDataTransaction(0x00);

	IO.writeCommandTransaction(0x02);     // POWER OFF
	_waitBusy();*/
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
		ESP_LOGD(tag, ">> Busy");
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

