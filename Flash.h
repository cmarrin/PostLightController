// Copyright Chris Marrin 2021
//
// Flash Class
//
// This class flashes the lights a given number of times (for error output)

#pragma once

#include "Effect.h"

class Flash : public Effect
{
public:
	Flash(Adafruit_NeoPixel* pixels) : Effect(pixels) { }
	virtual ~Flash() { }
	
	virtual int32_t loop() override;
		
private:	
	uint32_t _lastFlash = 0;
	bool _on = false;
	uint8_t _count = 0;
};
