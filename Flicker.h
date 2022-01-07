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
	    Speed(int _min, int _max, int _d) : stepsMin(_min), stepsMax(_max), delay(_d) { };
	    int stepsMin, stepsMax, delay;
	};

	static constexpr int _incMin = 1;          	// minimum pixel increment/decrement
	static constexpr int _incMax = 10;        	// maximum pixel increment/decrement
	static constexpr int _brightnessMin = 100;  // minimum starting brightness in pixels
	static constexpr int _minVal = 40;         	// lowest allowed val amount in pixels
	
	LED* _leds = nullptr;

	uint8_t _speed = 0;
	int _stepsMin = 0;
	int _stepsMax = 0;
	
	static const Speed _speedTable[8];
};
