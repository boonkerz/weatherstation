#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sdkconfig.h"
#include "Task.h"
#include "I2C.h"
#include "SPI.h"

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

#define swap(a, b) { int16_t t = a; a = b; b = t; }
/*******************************************************************/



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

static int EPD_OFFSET = 0;
static propFont	fontChar;

class DisplayBase : public Task {
private:
	int _width = 0;
	int _height = 0;

protected:
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

	Font_t cfont;					// Current font structure
	uint8_t image_debug;

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
	virtual void _drawPixel(int x, int y, uint8_t val);
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
	int _rotatePropChar(int x, int y, int offset);
public:
	void pushColorRep(int x1, int y1, int x2, int y2, color_t color);
};
