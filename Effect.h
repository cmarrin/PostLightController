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

	virtual void init(const uint8_t* buf = nullptr, uint32_t size = 0)
	{
		_buf = buf;
		_size = size;
	}
	
	// Return delay in ms
	virtual int32_t loop() = 0;
		
	// Return a float with a random number between min and max.
	// Multiply min and max by 100 and then divide the result 
	// to give 2 decimal digits of precision
	float randomFloat(uint8_t min, uint8_t max)
	{
		return float(random(int32_t(min) * 100, int32_t(max) * 100)) / 100;
	}
	
protected:
	static uint8_t cmdParamToValue(uint8_t param) { return (param - 0x30) & 0x3f; }
	
	Adafruit_NeoPixel* _pixels = nullptr;
	const uint8_t* _buf = nullptr;
	uint32_t _size = 0;
	
private:
};
