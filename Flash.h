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
	
	virtual void init(const uint8_t* buf, uint32_t size) override;
	virtual int32_t loop() override;
		
private:
	Color _color;
	uint8_t _count;
	uint32_t _duration; // in ms
	
	uint32_t _lastFlash = 0;
	bool _on = false;
	uint8_t _countCompleted = 0;
};
