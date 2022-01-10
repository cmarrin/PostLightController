// Copyright Chris Marrin 2021
//
// Flicker Class
//
// This class flickers the LEDs according to input params

#pragma once

#include "Effect.h"

class Flicker : public Effect
{
public:
	Flicker(Adafruit_NeoPixel* pixels);
	virtual ~Flicker();
	
	// Param3, lower 3 bits are speed
	virtual void init(const uint8_t* buf, uint32_t size) override;
	virtual int32_t loop() override;
		
private:
	struct LED {
	    float off = 0;
	    float inc = 0;
	    float lim = 0;
	};

	struct Speed {
	    Speed() { }
	    Speed(uint8_t _min, uint8_t _max, uint8_t _d) : stepsMin(_min), stepsMax(_max), delay(_d) { };
	    uint8_t stepsMin, stepsMax, delay;
	};

	static constexpr uint8_t IncMin = 1;          	// minimum pixel increment/decrement
	static constexpr uint8_t IncMax = 10;        	// maximum pixel increment/decrement
	static constexpr uint8_t BrightnessMin = 100;	// minimum starting brightness in pixels
	static constexpr uint8_t ValMin = 40;         	// lowest allowed val amount in pixels
	
	LED* _leds = nullptr;

	Color _color;
	
	static const Speed _speedTable[8];
};
