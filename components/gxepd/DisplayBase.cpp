#include "DisplayBase.h"


// User defined call-back function to input JPEG data from file
//---------------------
static UINT tjd_input (
	JDEC* jd,		// Decompression object
	BYTE* buff,		// Pointer to the read buffer (NULL:skip)
	UINT nd			// Number of bytes to read/skip from input stream
)
{
	int rb = 0;
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;

	if (buff) {	// Read nd bytes from the input strem
		rb = fread(buff, 1, nd, dev->fhndl);
		return rb;	// Returns actual number of bytes read
	}
	else {	// Remove nd bytes from the input stream
		if (fseek(dev->fhndl, nd, SEEK_CUR) >= 0) return nd;
		else return 0;
	}
}

// User defined call-back function to input JPEG data from memory buffer
//-------------------------
static UINT tjd_buf_input (
	JDEC* jd,		// Decompression object
	BYTE* buff,		// Pointer to the read buffer (NULL:skip)
	UINT nd			// Number of bytes to read/skip from input stream
)
{
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;
	if (!dev->membuff) return 0;
	if (dev->bufptr >= (dev->bufsize + 2)) return 0; // end of stream

	if ((dev->bufptr + nd) > (dev->bufsize + 2)) nd = (dev->bufsize + 2) - dev->bufptr;

	if (buff) {	// Read nd bytes from the input strem
		memcpy(buff, dev->membuff + dev->bufptr, nd);
		dev->bufptr += nd;
		return nd;	// Returns number of bytes read
	}
	else {	// Remove nd bytes from the input stream
		dev->bufptr += nd;
		return nd;
	}
}

// User defined call-back function to output RGB bitmap to display device
//----------------------
static UINT tjd_output (
	JDEC* jd,		// Decompression object of current session
	void* bitmap,	// Bitmap data to be output
	JRECT* rect		// Rectangular region to output
)
{
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;

	// ** Put the rectangular into the display device **
	int x;
	int y;
	int dleft, dtop, dright, dbottom;
	BYTE *src = (BYTE*)bitmap;

	int left = rect->left + dev->x;
	int top = rect->top + dev->y;
	int right = rect->right + dev->x;
	int bottom = rect->bottom + dev->y;

	if ((left > dispWin.x2) || (top > dispWin.y2)) return 1;	// out of screen area, return
	if ((right < dispWin.x1) || (bottom < dispWin.y1)) return 1;// out of screen area, return

	if (left < dispWin.x1) dleft = dispWin.x1;
	else dleft = left;
	if (top < dispWin.y1) dtop = dispWin.y1;
	else dtop = top;
	if (right > dispWin.x2) dright = dispWin.x2;
	else dright = right;
	if (bottom > dispWin.y2) dbottom = dispWin.y2;
	else dbottom = bottom;

	if ((dleft > dispWin.x2) || (dtop > dispWin.y2)) return 1;		// out of screen area, return
	if ((dright < dispWin.x1) || (dbottom < dispWin.y1)) return 1;	// out of screen area, return

	uint32_t len = ((dright-dleft+1) * (dbottom-dtop+1));	// calculate length of data

	float gs_clr = 0;
	uint8_t rgb_color[3];
	uint8_t last_lvl, i;
	uint8_t pix;
	if ((len > 0) && (len <= JPG_IMAGE_LINE_BUF_SIZE)) {
		for (y = top; y <= bottom; y++) {
			for (x = left; x <= right; x++) {
				// Clip to display area
				if ((x >= dleft) && (y >= dtop) && (x <= dright) && (y <= dbottom)) {
					// Directly convert color to 4-bit gray scale
					pix = 0;
					pix |= ((*src++) >> 4) & 0x08;
					pix |= ((*src++) >> 5) & 0x06;
					pix |= ((*src++) >> 7);
					pix ^= 0x0F;


					/* Convert rgb color to gray scale
					memcpy(rgb_color, src, 3);
					src += 3;
				    gs_clr = (GS_FACT_R * rgb_color[0]) + (GS_FACT_G * rgb_color[1]) + (GS_FACT_B * rgb_color[2]);
				    if (gs_clr > 255) gs_clr = 255;
				    // Use only 4 bits & invert
				    //pix = ((uint8_t)gs_clr >> 4) ^ 0x0F;
				    pix = (uint8_t)gs_clr;

				    // Using gray scale lookup table
				    last_lvl = 0;
				    i = 0;
				    for (i=0; i<16; i++) {
				    	if ((pix > last_lvl) && (pix <= lvl_buf_jpg[i])) {
				    		pix = 15 - i;
					        last_lvl = lvl_buf[i];
					        break;
				        }
				        last_lvl = lvl_buf[i];
				    }
					*/
				    gs_disp_buffer[(y * EPD_DISPLAY_WIDTH) + x] = pix;
		    		gs_used_shades |= (1 << pix);
				}
				else src += 3; // skip
			}
		}
	}
	else {
		printf("Data size error: %d jpg: (%d,%d,%d,%d) disp: (%d,%d,%d,%d)\r\n", len, left,top,right,bottom, dleft,dtop,dright,dbottom);
		return 0;  // stop decompression
	}

	return 1;	// Continue to decompression
}


static char tag[] = "DisplayBaseClass";

void DisplayBase::pushColorRep(int x1, int y1, int x2, int y2, color_t color)
{
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
	  swapMethod(y0, y1); swapMethod(x0, x1);
  }
  if (y1 > y2) {
	  swapMethod(y2, y1); swapMethod(x2, x1);
  }
  if (y0 > y1) {
	  swapMethod(y0, y1); swapMethod(x0, x1);
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
    if(a > b) swapMethod(a,b);
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
    if(a > b) swapMethod(a,b);
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
	  swapMethod(x0, y0);
	  swapMethod(x1, y1);
  }
  if (x0 > x1) {
	  swapMethod(x0, x1);
	  swapMethod(y0, y1);
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
}

void DisplayBase::_drawRect(uint16_t x1,uint16_t y1,uint16_t w,uint16_t h, color_t color) {
  _drawFastHLine(x1,y1,w, color);
  _drawFastVLine(x1+w-1,y1,h, color);
  _drawFastHLine(x1,y1+h-1,w, color);
  _drawFastVLine(x1,y1,h, color);
}

int DisplayBase::_printProportionalChar(int x, int y) {
	uint8_t ch = 0;
	int i, j, char_width;

	char_width = ((fontChar.width > fontChar.xDelta) ? fontChar.width : fontChar.xDelta);
	int cx, cy;
	if (!font_transparent) _fillRect(x, y, char_width+1, cfont.y_size, _bg);
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

int DisplayBase::_7seg_width()
{
	return (2 * (2 * cfont.y_size + 1)) + cfont.x_size;
}

//-----------------------
int DisplayBase::_7seg_height()
{
	return (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size);
}

uint8_t DisplayBase::_getCharPtr(uint8_t c) {
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



//============================================================================
void DisplayBase::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color) {
	_fillRect(x+dispWin.x1, y+dispWin.y1, w, h, color);
}

//======================================================================
void DisplayBase::drawFastVLine(int16_t x, int16_t y, int16_t h, color_t color) {
	_drawFastVLine(x+dispWin.x1, y+dispWin.y1, h, color);
}

//======================================================================
void DisplayBase::drawFastHLine(int16_t x, int16_t y, int16_t w, color_t color) {
	_drawFastHLine(x+dispWin.x1, y+dispWin.y1, w, color);
}

void DisplayBase::_draw7seg(int16_t x, int16_t y, int8_t num, int16_t w, int16_t l, color_t color) {
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

int DisplayBase::_getStringWidth(char* str)
{
    int strWidth = 0;

	if (cfont.bitmap == 2) strWidth = ((_7seg_width()+2) * strlen(str)) - 2;	// 7-segment font
	else if (cfont.x_size != 0) strWidth = strlen(str) * cfont.x_size;			// fixed width font
	else {
		// calculate the width of the string of proportional characters
		char* tempStrptr = str;
		while (*tempStrptr != 0) {
			if (_getCharPtr(*tempStrptr++)) {
				strWidth += (((fontChar.width > fontChar.xDelta) ? fontChar.width : fontChar.xDelta) + 1);
			}
		}
		strWidth--;
	}
	return strWidth;
}

void DisplayBase::drawText(char *st, int x, int y) {
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
				if (_getCharPtr(ch)) tmpw = fontChar.xDelta;
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

void DisplayBase::_drawPixel(int x, int y, uint8_t val)
{

}

int DisplayBase::getFontHeight()
{
  if (cfont.bitmap == 1) return cfont.y_size;			// Bitmap font
  else if (cfont.bitmap == 2) return _7seg_height();	// 7-segment font
  return 0;
}



void DisplayBase::setFont(uint8_t font, const char *font_file)
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

void DisplayBase::_getMaxWidthHeight()
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

int DisplayBase::drawImageJpg(int x, int y, uint8_t scale, char *fname, uint8_t *buf, int size)
{
	JPGIODEV dev;
    struct stat sb;
	char *work = NULL;		// Pointer to the working buffer (must be 4-byte aligned)
	UINT sz_work = 3800;	// Size of the working buffer (must be power of 2)
	JDEC jd;				// Decompression object (70 bytes)
	JRESULT rc;
	int res = -10;

    if (fname == NULL) {
    	// image from buffer
    	dev.fhndl = NULL;
        dev.membuff = buf;
        dev.bufsize = size;
        dev.bufptr = 0;
    }
    else {
    	// image from file
        dev.membuff = NULL;
        dev.bufsize = 0;
        dev.bufptr = 0;

        if (stat(fname, &sb) != 0) {
        	if (image_debug) printf("File error: %ss\r\n", strerror(errno));
        	res = -11;
            goto exit;
        }

        dev.fhndl = fopen(fname, "r");
        if (!dev.fhndl) {
        	if (image_debug) printf("Error opening file: %s\r\n", strerror(errno));
        	res = -12;
            goto exit;
        }
    }

	if (scale > 3) scale = 3;

	work = malloc(sz_work);
	if (work) {
		if (dev.membuff) rc = jd_prepare(&jd, tjd_buf_input, (void *)work, sz_work, &dev);
		else rc = jd_prepare(&jd, tjd_input, (void *)work, sz_work, &dev);
		if (rc == JDR_OK) {
			if (x == CENTER) x = ((dispWin.x2 - dispWin.x1 + 1 - (int)(jd.width >> scale)) / 2) + dispWin.x1;
			else if (x == RIGHT) x = dispWin.x2 + 1 - (int)(jd.width >> scale);

			if (y == CENTER) y = ((dispWin.y2 - dispWin.y1 + 1 - (int)(jd.height >> scale)) / 2) + dispWin.y1;
			else if (y == BOTTOM) y = dispWin.y2 + 1 - (int)(jd.height >> scale);

			if (x < ((dispWin.x2-1) * -1)) x = (dispWin.x2-1) * -1;
			if (y < ((dispWin.y2-1)) * -1) y = (dispWin.y2-1) * -1;
			if (x > (dispWin.x2-1)) x = dispWin.x2 - 1;
			if (y > (dispWin.y2-1)) y = dispWin.y2-1;

			dev.x = x;
			dev.y = y;

			// Start to decode the JPEG file
			rc = jd_decomp(&jd, tjd_output, scale);

			if (rc != JDR_OK) {
				if (image_debug) printf("jpg decompression error %d\r\n", rc);
				res = rc * -1;
			}
			res = 0;
			if (image_debug) printf("Jpg size: %dx%d, position; %d,%d, scale: %d, bytes used: %d\r\n", jd.width, jd.height, x, y, scale, jd.sz_pool);
		}
		else {
			if (image_debug) printf("jpg prepare error %d\r\n", rc);
			res = rc * -1;
		}
	}
	else {
		if (image_debug) printf("work buffer allocation error\r\n");
		res = -13;
	}

exit:
	if (work) free(work);  // free work buffer
    if (dev.fhndl) fclose(dev.fhndl);  // close input file
    return res;
}
