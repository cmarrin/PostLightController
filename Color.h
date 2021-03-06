/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Color Class
//
// This class holds a color in hsv format (floats 0 to 1)

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#ifdef ARDUINO
#include <Adafruit_NeoPixel.h>
static uint32_t gamma(uint32_t rgb) { return Adafruit_NeoPixel::gamma32(rgb); }
#else
static uint32_t gamma(uint32_t rgb) { return rgb; }
#endif

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
