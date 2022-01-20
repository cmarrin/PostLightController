// Copyright Chris Marrin 2021
//
// InterpretedEffect Class

#include "InterpretedEffect.h"

InterpretedEffect::InterpretedEffect(Adafruit_NeoPixel* pixels)
	: Effect(pixels)
	, _device(pixels)
{
}

bool
InterpretedEffect::init(uint8_t cmd, const uint8_t* buf, uint32_t size)
{
	Effect::init(cmd, buf, size);
	
	if (!_device.init(cmd, buf, size)) {
		return false;
	}

	Serial.print("InterpretedEffect started: cmd='");
	Serial.print(char(cmd));
	Serial.println("'");

	return true;
}

int32_t
InterpretedEffect::loop()
{
    return _device.loop();
}
