#include "DisplayBase.h"

static char tag[] = "DisplayBaseClass";

void DisplayBase::pushColorRep(int x1, int y1, int x2, int y2, color_t color)
{
	ESP_LOGD(tag, ">> pushColorRep %d %d %d %d %d",_gs, x1, y1, x2, y2);
	if (_gs) {
		color &= 0x01;
	}else {
		color &= 0x0F;
		for (int y=y1; y<=y2; y++) {
			for (int x = x1; x<=x2; x++){
				_drawPixel(x, y, color);
			}
		}
	}
}

void DisplayBase::_drawPixel(int x, int y, uint8_t val)
{
}


void DisplayBase::_drawFastVLine(int16_t x, int16_t y, int16_t h, color_t color) {
	// clipping
	if ((x < dispWin.x1) || (x > dispWin.x2) || (y > dispWin.y2)) return;
	if (y < dispWin.y1) {
		h -= (dispWin.y1 - y);
		y = dispWin.y1;
	}
	if (h < 0) h = 0;
	if ((y + h) > (dispWin.y2+1)) h = dispWin.y2 - y + 1;
	if (h == 0) h = 1;
	pushColorRep(x, y, x, y+h-1, color);
}

//--------------------------------------------------------------------------
void DisplayBase::_drawFastHLine(int16_t x, int16_t y, int16_t w, color_t color) {
	// clipping
	if ((y < dispWin.y1) || (x > dispWin.x2) || (y > dispWin.y2)) return;
	if (x < dispWin.x1) {
		w -= (dispWin.x1 - x);
		x = dispWin.x1;
	}
	if (w < 0) w = 0;
	if ((x + w) > (dispWin.x2+1)) w = dispWin.x2 - x + 1;
	if (w == 0) w = 1;

	pushColorRep(x, y, x+w-1, y, color);
}

void DisplayBase::_fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color)
{
  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    _drawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  int32_t
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    _drawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    _drawFastHLine(a, y, b-a+1, color);
  }
}

void DisplayBase::_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color)
{
	_drawLine(x0, y0, x1, y1, color);
	_drawLine(x1, y1, x2, y2, color);
	_drawLine(x2, y2, x0, y0, color);
}

void DisplayBase::_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color)
{
  if (x0 == x1) {
	  if (y0 <= y1) _drawFastVLine(x0, y0, y1-y0, color);
	  else _drawFastVLine(x0, y1, y0-y1, color);
	  return;
  }
  if (y0 == y1) {
	  if (x0 <= x1) _drawFastHLine(x0, y0, x1-x0, color);
	  else _drawFastHLine(x1, y0, x0-x1, color);
	  return;
  }

  int steep = 0;
  if (abs(y1 - y0) > abs(x1 - x0)) steep = 1;
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }
  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx = x1 - x0, dy = abs(y1 - y0);;
  int16_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

  if (y0 < y1) ystep = 1;

  // Split into steep and not steep for FastH/V separation
  if (steep) {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) _drawPixel(y0, xs, color);
        else _drawFastVLine(y0, xs, dlen, color);
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) _drawFastVLine(y0, xs, dlen, color);
  }
  else
  {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) _drawPixel(xs, y0, color);
        else _drawFastHLine(xs, y0, dlen, color);
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) _drawFastHLine(xs, y0, dlen, color);
  }
}

void DisplayBase::_printChar(uint8_t c, int x, int y) {
	ESP_LOGD(tag, ">> printChar begin %d %d %d", x, y, _fg );
	uint8_t i, j, ch, fz, mask;
	uint16_t k, temp, cx, cy;

	// fz = bytes per char row
	fz = cfont.x_size/8;
	if (cfont.x_size % 8) fz++;

	// get character position in buffer
	temp = ((c-cfont.offset)*((fz)*cfont.y_size))+4;

	if (!font_transparent) _fillRect(x, y, cfont.x_size, cfont.y_size, _bg);

	for (j=0; j<cfont.y_size; j++) {
		for (k=0; k < fz; k++) {
			ch = cfont.font[temp+k];
			mask=0x80;
			for (i=0; i<8; i++) {
				if ((ch & mask) !=0) {
					cx = (uint16_t)(x+i+(k*8));
					cy = (uint16_t)(y+j);
					_drawPixel(cx, cy, _fg);
				}
				mask >>= 1;
			}
		}
		temp += (fz);
	}
}

void DisplayBase::_rotateChar(uint8_t c, int x, int y, int pos) {
  uint8_t i,j,ch,fz,mask;
  uint16_t temp;
  int newx,newy;
  double radian = font_rotate*0.0175;
  float cos_radian = cos(radian);
  float sin_radian = sin(radian);
  int zz;

  if( cfont.x_size < 8 ) fz = cfont.x_size;
  else fz = cfont.x_size/8;
  temp=((c-cfont.offset)*((fz)*cfont.y_size))+4;

  for (j=0; j<cfont.y_size; j++) {
    for (zz=0; zz<(fz); zz++) {
      ch = cfont.font[temp+zz];
      mask = 0x80;
      for (i=0; i<8; i++) {
        newx=(int)(x+(((i+(zz*8)+(pos*cfont.x_size))*cos_radian)-((j)*sin_radian)));
        newy=(int)(y+(((j)*cos_radian)+((i+(zz*8)+(pos*cfont.x_size))*sin_radian)));

        if ((ch & mask) != 0) _drawPixel(newx,newy,_fg);
        else if (!font_transparent) _drawPixel(newx,newy,_bg);
        mask >>= 1;
      }
    }
    temp+=(fz);
  }
  // calculate x,y for the next char
  EPD_X = (int)(x + ((pos+1) * cfont.x_size * cos_radian));
  EPD_Y = (int)(y + ((pos+1) * cfont.x_size * sin_radian));
}

//-----------------------------------------------------------------------------------------------
void DisplayBase::_barVert(int16_t x, int16_t y, int16_t w, int16_t l, color_t color, color_t outline) {
  _fillTriangle(x+1, y+2*w, x+w, y+w+1, x+2*w-1, y+2*w, color);
  _fillTriangle(x+1, y+2*w+l+1, x+w, y+3*w+l, x+2*w-1, y+2*w+l+1, color);
  _fillRect(x, y+2*w+1, 2*w+1, l, color);
  if (cfont.offset) {
    _drawTriangle(x+1, y+2*w, x+w, y+w+1, x+2*w-1, y+2*w, outline);
    _drawTriangle(x+1, y+2*w+l+1, x+w, y+3*w+l, x+2*w-1, y+2*w+l+1, outline);
    _drawRect(x, y+2*w+1, 2*w+1, l, outline);
  }
}

//----------------------------------------------------------------------------------------------
void DisplayBase::_barHor(int16_t x, int16_t y, int16_t w, int16_t l, color_t color, color_t outline) {
  _fillTriangle(x+2*w, y+2*w-1, x+w+1, y+w, x+2*w, y+1, color);
  _fillTriangle(x+2*w+l+1, y+2*w-1, x+3*w+l, y+w, x+2*w+l+1, y+1, color);
  _fillRect(x+2*w+1, y, l, 2*w+1, color);
  if (cfont.offset) {
    _drawTriangle(x+2*w, y+2*w-1, x+w+1, y+w, x+2*w, y+1, outline);
    _drawTriangle(x+2*w+l+1, y+2*w-1, x+3*w+l, y+w, x+2*w+l+1, y+1, outline);
    _drawRect(x+2*w+1, y, l, 2*w+1, outline);
  }
}

void DisplayBase::_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color) {
	ESP_LOGD(tag, ">> _fillRect begin");
	if ((x >= dispWin.x2) || (y > dispWin.y2)) return;

	if (x < dispWin.x1) {
		w -= (dispWin.x1 - x);
		x = dispWin.x1;
	}
	if (y < dispWin.y1) {
		h -= (dispWin.y1 - y);
		y = dispWin.y1;
	}
	if (w < 0) w = 0;
	if (h < 0) h = 0;

	if ((x + w) > (dispWin.x2+1)) w = dispWin.x2 - x + 1;
	if ((y + h) > (dispWin.y2+1)) h = dispWin.y2 - y + 1;
	if (w == 0) w = 1;
	if (h == 0) h = 1;
	pushColorRep(x, y, x+w-1, y+h-1, color);
	ESP_LOGD(tag, ">> _fillRect end");
}

void DisplayBase::_drawRect(uint16_t x1,uint16_t y1,uint16_t w,uint16_t h, color_t color) {
  _drawFastHLine(x1,y1,w, color);
  _drawFastVLine(x1+w-1,y1,h, color);
  _drawFastHLine(x1,y1+h-1,w, color);
  _drawFastVLine(x1,y1,h, color);
}

int DisplayBase::_printProportionalChar(int x, int y) {
	ESP_LOGD(tag, ">> printProportionalChar begin %d %d %d", x, y, _fg);
	uint8_t ch = 0;
	int i, j, char_width;

	char_width = ((fontChar.width > fontChar.xDelta) ? fontChar.width : fontChar.xDelta);
	int cx, cy;

	if (!font_transparent) _fillRect(x, y, char_width+1, cfont.y_size, _bg);
	ESP_LOGD(tag, ">> printProportionalChar middle %d %d %d", x, y, _fg);
	// draw Glyph
	uint8_t mask = 0x80;
	for (j=0; j < fontChar.height; j++) {
		for (i=0; i < fontChar.width; i++) {
			if (((i + (j*fontChar.width)) % 8) == 0) {
				mask = 0x80;
				ch = cfont.font[fontChar.dataPtr++];
			}

			if ((ch & mask) !=0) {
				cx = (uint16_t)(x+fontChar.xOffset+i);
				cy = (uint16_t)(y+j+fontChar.adjYOffset);
				_drawPixel(cx, cy, _fg);
			}
			mask >>= 1;
		}
	}

	return char_width;
}

int DisplayBase::_rotatePropChar(int x, int y, int offset) {
  uint8_t ch = 0;
  double radian = font_rotate * DEG_TO_RAD;
  float cos_radian = cos(radian);
  float sin_radian = sin(radian);

  uint8_t mask = 0x80;
  for (int j=0; j < fontChar.height; j++) {
    for (int i=0; i < fontChar.width; i++) {
      if (((i + (j*fontChar.width)) % 8) == 0) {
        mask = 0x80;
        ch = cfont.font[fontChar.dataPtr++];
      }

      int newX = (int)(x + (((offset + i) * cos_radian) - ((j+fontChar.adjYOffset)*sin_radian)));
      int newY = (int)(y + (((j+fontChar.adjYOffset) * cos_radian) + ((offset + i) * sin_radian)));

      if ((ch & mask) != 0) _drawPixel(newX,newY,_fg);
      else if (!font_transparent) _drawPixel(newX,newY,_bg);

      mask >>= 1;
    }
  }

  return fontChar.xDelta+1;
}
