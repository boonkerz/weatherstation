#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sdkconfig.h"
#include "I2C.h"
#include "SPI.h"
#include <sys/stat.h>
#include "rom/tjpgd.h"
#include <errno.h>


/**************************************************************/

#define DEG_TO_RAD 0.01745329252
#define RAD_TO_DEG 57.295779513
#define deg_to_rad 0.01745329252 + 3.14159265359

typedef uint8_t color_t;

typedef struct {
	uint16_t        x1;
	uint16_t        y1;
	uint16_t        x2;
	uint16_t        y2;
} dispWin_t;

typedef struct {
	uint8_t 	*font;
	uint8_t 	x_size;
	uint8_t 	y_size;
	uint8_t	    offset;
	uint16_t	numchars;
    uint16_t	size;
	uint8_t 	max_x_size;
    uint8_t     bitmap;
	color_t     color;
} Font_t;


// Buffer is created during jpeg decode for sending data
// Total size of the buffer is  2 * (JPG_IMAGE_LINE_BUF_SIZE * 3)
// The size must be multiple of 256 bytes !!
#define JPG_IMAGE_LINE_BUF_SIZE 512

// --- Constants for ellipse function ---
#define EPD_ELLIPSE_UPPER_RIGHT 0x01
#define EPD_ELLIPSE_UPPER_LEFT  0x02
#define EPD_ELLIPSE_LOWER_LEFT  0x04
#define EPD_ELLIPSE_LOWER_RIGHT 0x08

// Constants for Arc function
// number representing the maximum angle (e.g. if 100, then if you pass in start=0 and end=50, you get a half circle)
// this can be changed with setArcParams function at runtime
#define DEFAULT_ARC_ANGLE_MAX 360
// rotational offset in degrees defining position of value 0 (-90 will put it at the top of circle)
// this can be changed with setAngleOffset function at runtime
#define DEFAULT_ANGLE_OFFSET -90

#define PI 3.14159265359

#define MIN_POLIGON_SIDES	3
#define MAX_POLIGON_SIDES	60

// === Color names constants ===
#define EPD_BLACK 15
#define EPD_WHITE 0

// === Color invert constants ===
#define INVERT_ON		1
#define INVERT_OFF		0

// === Screen orientation constants ===
#define LANDSCAPE_0		1
#define LANDSCAPE_180	2

// === Special coordinates constants ===
#define CENTER	-9003
#define RIGHT	-9004
#define BOTTOM	-9004

#define LASTX	7000
#define LASTY	8000

// === Embedded fonts constants ===
#define DEFAULT_FONT	0
#define DEJAVU18_FONT	1
#define DEJAVU24_FONT	2
#define UBUNTU16_FONT	3
#define COMIC24_FONT	4
#define MINYA24_FONT	5
#define TOONEY32_FONT	6
#define SMALL_FONT		7
#define FONT_7SEG		8
#define USER_FONT		9  // font will be read from file

#define swapMethod(a, b) { int16_t t = a; a = b; b = t; }
/*******************************************************************/

extern uint8_t tft_SmallFont[];
extern uint8_t tft_DefaultFont[];
extern uint8_t tft_Dejavu18[];
extern uint8_t tft_Dejavu24[];
extern uint8_t tft_Ubuntu16[];
extern uint8_t tft_Comic24[];
extern uint8_t tft_minya24[];
extern uint8_t tft_tooney32[];


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


/*******************************************************************/
typedef struct {
      uint8_t charCode;
      int adjYOffset;
      int width;
      int height;
      int xOffset;
      int xDelta;
      uint16_t dataPtr;
} propFont;

// User defined device identifier
typedef struct {
	FILE		*fhndl;			// File handler for input function
    int			x;				// image top left point X position
    int			y;				// image top left point Y position
    uint8_t		*membuff;		// memory buffer containing the image
    uint32_t	bufsize;		// size of the memory buffer
    uint32_t	bufptr;			// memory buffer current position
} JPGIODEV;


static int EPD_OFFSET = 0;

class DisplayBase {
private:

protected:

	int8_t _current_page = -1;
	uint8_t   orientation;		// current screen orientation
	uint16_t  font_rotate;   	// current font font_rotate angle (0~395)
	uint8_t   font_transparent;	// if not 0 draw fonts transparent
	uint8_t   font_forceFixed;  // if not zero force drawing proportional fonts with fixed width
	uint8_t   font_buffered_char;
	uint8_t   font_line_space;	// additional spacing between text lines; added to font height
	uint8_t   text_wrap;        // if not 0 wrap long text to the new line, else clip
	color_t   _fg = EPD_BLACK;            	// current foreground color for fonts
	color_t   _bg = EPD_WHITE;            	// current background for non transparent fonts
	dispWin_t dispWin;			// display clip window
	float	  _angleOffset;		// angle offset for arc, polygon and line by angle functions
	int _width = 0;
	int _height = 0;
	Font_t cfont;					// Current font structure
	uint8_t image_debug;
	propFont fontChar;
	int	EPD_X;					// X position of the next character after EPD_print() function
	int	EPD_Y;					// Y position of the next character after EPD_print() function
	uint8_t *gs_disp_buffer = NULL;
	uint8_t *disp_buffer = NULL;
	uint8_t *drawBuff = NULL;
	uint8_t *gs_drawBuff = NULL;

	uint8_t _gs = 0;

	uint16_t gs_used_shades = 0;

	void _drawFastVLine(int16_t x, int16_t y, int16_t h, color_t color);
	void _drawFastHLine(int16_t x, int16_t y, int16_t w, color_t color);
	void _fillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color);
	void _drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color);
	void _drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, color_t color);
	void _printChar(uint8_t c, int x, int y);
	void _rotateChar(uint8_t c, int x, int y, int pos);
	void _barVert(int16_t x, int16_t y, int16_t w, int16_t l, color_t color, color_t outline);
	void _barHor(int16_t x, int16_t y, int16_t w, int16_t l, color_t color, color_t outline);
	void _fillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color);
	void _drawRect(uint16_t x1,uint16_t y1,uint16_t w,uint16_t h, color_t color);
	int _printProportionalChar(int x, int y);
	virtual void  _drawPixel(int x, int y, uint8_t val);
	int _rotatePropChar(int x, int y, int offset);
	int _7seg_width();
	int _7seg_height();
	int _getStringWidth(char* str);
	uint8_t _getCharPtr(uint8_t c);
	void _draw7seg(int16_t x, int16_t y, int8_t num, int16_t w, int16_t l, color_t color);
	void _getMaxWidthHeight();
	void _send8pixel(uint8_t data);
public:
	void pushColorRep(int x1, int y1, int x2, int y2, color_t color);
	void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, color_t color);
	void drawFastHLine(int16_t x, int16_t y, int16_t w, color_t color);
	void drawFastVLine(int16_t x, int16_t y, int16_t h, color_t color);
	void drawText(char *st, int x, int y);
	void drawImageJpg(int x, int y, uint8_t scale, char *fname, uint8_t *buf, int size);
	void setFont(uint8_t font, const char *font_file);
	int getFontHeight();
};
