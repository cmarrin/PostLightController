// Copyright Chris Marrin 2021
//
// Color Class
//
// This class holds a color in hsv format (floats 0 to 1)

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#ifdef ARDUINO
#else
    #define PROGMEM
    #define pgm_read_byte(a) (*a)
#endif

// Taken from Adafruit_NeoPixel.h
/* 8-bit gamma-correction table. Copy & paste this snippet into a Python
   REPL to regenerate:
        import math
        gamma=2.6
        for x in range(256):
            print("{:3},".format(int(math.pow((x)/255.0,gamma)*255.0+0.5))),
            if x&15 == 15: print
*/
static const uint8_t PROGMEM _NeoPixelGammaTable[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,
    3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,
    6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,
    11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,
    17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,
    25,  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,  33,  34,  34,  35,
    36,  37,  38,  38,  39,  40,  41,  42,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
    64,  65,  66,  68,  69,  70,  71,  72,  73,  75,  76,  77,  78,  80,  81,
    82,  84,  85,  86,  88,  89,  90,  92,  93,  94,  96,  97,  99,  100, 102,
    103, 105, 106, 108, 109, 111, 112, 114, 115, 117, 119, 120, 122, 124, 125,
    127, 129, 130, 132, 134, 136, 137, 139, 141, 143, 145, 146, 148, 150, 152,
    154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182,
    184, 186, 188, 191, 193, 195, 197, 199, 202, 204, 206, 209, 211, 213, 215,
    218, 220, 223, 225, 227, 230, 232, 235, 237, 240, 242, 245, 247, 250, 252,
    255
};

static inline uint32_t gamma(uint32_t rgb)
{
    uint8_t *y = (uint8_t *) &rgb;
    for (uint8_t i = 0; i < 4; i++) {
        y[i] = pgm_read_byte(&_NeoPixelGammaTable[y[i]]);
    }
    return rgb;
}

class Color
{
public:
	Color() { }
	Color(uint8_t h, uint8_t s, uint8_t v, bool gammaCorrect = true)
	{
		setColor(h, s, v, gammaCorrect);
	}
	
	Color(float h, float s, float v, bool gammaCorrect = true)
	{
        _hue = h;
        _sat = s;
        _val = v;
        _gammaCorrect = gammaCorrect;
	}
	
	uint32_t rgb() const
	{
		uint16_t h = fmin(fmax(_hue * 65535.0f, 0.0f), 65535.0f);
		uint8_t s = fmin(fmax(_sat * 255.0f, 0.0f), 255.0f);
		uint8_t v = fmin(fmax(_val * 255.0f, 0.0f), 255.0f);
		uint32_t c = hsvToRGB(h, s, v);
		return _gammaCorrect ? gamma(c) : c;
	}
	
	float hue() const { return _hue; }
	float sat() const { return _sat; }
	float val() const { return _val; }
 
    void setHue(float h) { _hue = h; }
    void setSat(float s) { _sat = s; }
    void setVal(float v) { _val = v; }
	
private:
	void setColor(uint8_t a, uint8_t b, uint8_t c, bool gammaCorrect = true);
    
    static uint32_t hsvToRGB(uint16_t hue, uint8_t sat, uint8_t val);
	
	float _hue = 0;
	float _sat = 0;
	float _val = 0;
	bool _gammaCorrect = true;
};
