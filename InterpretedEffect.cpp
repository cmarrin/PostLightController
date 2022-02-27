// Copyright Chris Marrin 2021
//
// InterpretedEffect Class

#include "InterpretedEffect.h"

InterpretedEffect::InterpretedEffect(arly::NativeModule** mod, uint32_t modSize, Adafruit_NeoPixel* pixels)
	: Effect(pixels)
	, _device(mod, modSize, pixels)
{
}

bool
InterpretedEffect::init(uint8_t cmd, const uint8_t* buf, uint32_t size)
{
	Effect::init(cmd, buf, size);
	
    char c[2];
    c[0] = cmd;
    c[1] = '\0';
	if (!_device.init(c, buf, size)) {
		return false;
	}

	Serial.print(F("InterpretedEffect started: cmd='"));
	Serial.print(char(cmd));
	Serial.println(F("'"));

	return true;
}

int32_t
InterpretedEffect::loop()
{
    return _device.loop();
}
