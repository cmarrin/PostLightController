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
	
	virtual void init(const uint8_t* buf, uint32_t size) override;
	virtual int32_t loop() override;
	
	void setColor(const Color& color);
			
private:
	Color _color;
};
