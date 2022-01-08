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
	Effect(Adafruit_NeoPixel* pixels) : _pixels(pixels) { }
	
	virtual ~Effect() { }
	
	// Params are:
	//
	//		buf[0] - hue
	//		buf[1] - sat
	//		buf[2] - val
	//		buf[3] - param1
	//		buf[4] - param2
	//
	virtual void init(const uint8_t* buf = nullptr, uint32_t size = 0);
	
	// Return delay in ms
	virtual int32_t loop() = 0;
		
protected:
	// All values are floats from 0 to 1. Hue is converted to 0-65535, sat and val
	// are converted to 0-255
	uint32_t HSVToRGB(float h, float s, float v, bool gammaCorrect = true)
	{
		uint32_t c = _pixels->ColorHSV(round(h * 65535), round(s * 255), min(round(v * 255), 255));
		if (gammaCorrect) {
			c = _pixels->gamma32(c);
		}
	    return c;
	}
	
	static uint8_t cmdParamToValue(uint8_t param) { return (param - 0x30) & 0x3f; }
	
	Adafruit_NeoPixel* _pixels = nullptr;
	float _hue = 0;
	float _sat = 0;
	float _val = 0;
	uint8_t _param1 = 0;
	uint8_t _param2 = 0;
	
private:
};
