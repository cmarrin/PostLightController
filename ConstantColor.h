// Copyright Chris Marrin 2021
//
// ConstantColor Class
//
// This class sets a constant color on all lights

#pragma once

#include "Effect.h"

class ConstantColor : public Effect
{
public:
	ConstantColor(Adafruit_NeoPixel* pixels);
	virtual ~ConstantColor();
	
	virtual void init(uint8_t param1, uint8_t param2, uint8_t param3) override;
	virtual uint32_t loop() override;
		
private:
};
