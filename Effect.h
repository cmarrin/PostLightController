// Copyright Chris Marrin 2021
//
// Effect Base Class
//
// This class is the base class for all effects

#pragma once

#include <Adafruit_NeoPixel.h>

class Effect
{
public:
	Effect(Adafruit_NeoPixel* pixels, uint32_t color = 0) : _pixels(pixels)
	{
		_hue = float((color >> 6) & 0x1f) / 32;
		_sat = float((color >> 3) & 0x7) / 8;
		_val = float(color & 0x07) / 8;
	}
	
	virtual ~Effect() { }
	
	// Return delay in ms
	virtual uint32_t loop() = 0;
		
	static uint16_t HSVParamsToPackedHSV(uint8_t h, uint8_t s, uint8_t v)
	{
		uint16_t retval = min(h, 31);
		retval = (retval << 3) + min(s, 7);
		retval = (retval << 3) + min(v, 7);
		return retval;
	}

protected:
	// All values are floats from 0 to 1. Hue is converted to 0-65535, sat and val
	// are converted to 0-255
	uint32_t HSVToRGB(float h, float s, float v)
	{
	    return _pixels->gamma32(_pixels->ColorHSV(round(h * 65535), round(s * 255), min(round(v * 255), 255)));
	}
	
	Adafruit_NeoPixel* _pixels = nullptr;
	float _hue = 0;
	float _sat = 0;
	float _val = 0;
};
