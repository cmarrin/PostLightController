// Copyright Chris Marrin 2021
//
// Flicker Class
//
// This class flickers the LEDs according to input params

#pragma once

#include "Color.h"
#include "Effect.h"

class Flicker : public Effect
{
public:
	Flicker(Adafruit_NeoPixel* pixels);
	virtual ~Flicker();
	
	// Param3, lower 3 bits are speed
	virtual bool init(uint8_t cmd, const uint8_t* buf, uint32_t size) override;
	virtual int32_t loop() override;
		
private:
	struct LED {
	    float off = 0;
	    float inc = 0;
	    float lim = 0;
	};

	struct Speed {
	    Speed() { }
	    Speed(int32_t _min, int32_t _max, int32_t _d) : stepsMin(_min), stepsMax(_max), delay(_d) { };
	    int32_t stepsMin, stepsMax, delay;
	};

	static constexpr int32_t IncMin = 1;          	// minimum pixel increment/decrement
	static constexpr int32_t IncMax = 10;        	// maximum pixel increment/decrement
	static constexpr float BrightnessMin = 0.4;		// minimum starting brightness in pixels
	static constexpr float ValMin = 0.15;         	// lowest allowed val amount in pixels
	
	LED* _leds = nullptr;

	Color _color;
	
	static const Speed _speedTable[8];
};
