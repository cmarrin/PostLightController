/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "InterpretedEffect.h"

InterpretedEffect::InterpretedEffect(clvr::NativeModule** mod, uint32_t modSize, Adafruit_NeoPixel* pixels)
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
