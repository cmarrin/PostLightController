// Copyright Chris Marrin 2021
//
// Effect Base Class
//
// This class is the base class for all effects

#pragma once

#include <stdint.h>

class Adafruit_NeoPixel;

class Effect
{
public:
	Effect(Adafruit_NeoPixel* pixels) : _pixels(pixels) { }
	
	virtual ~Effect() { }

	virtual bool init(uint8_t cmd, const uint8_t* buf = nullptr, uint32_t size = 0)
	{
		(void) cmd;
		_buf = buf;
		_size = size;
		return true;
	}
	
	// Return delay in ms
	virtual int32_t loop() = 0;
		
protected:
	static uint8_t cmdParamToValue(uint8_t param) { return (param - 0x30) & 0x3f; }
	
	Adafruit_NeoPixel* _pixels = nullptr;
	const uint8_t* _buf = nullptr;
	uint32_t _size = 0;
	
private:
};
