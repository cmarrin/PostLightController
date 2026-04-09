/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Flash.h"

#include "PostLightController.h"
#include "System.h"

bool
Flash::init(uint8_t h, uint8_t s, uint8_t v, uint8_t count, uint16_t duration)
{
    // Incoming hue is 0-255, hsvToRGB expects 0-65535
    mil::Graphics::hsvToRGB(_red, _green, _blue, uint16_t(h) * 256, s, v);
	_countCompleted = 0;
    _count = count;
    _duration = uint16_t(duration) * 100;
    _lastFlash = mil::System::millis();

    // If we will be flashing (count != 0) then start with the lights off.
    // Otherwise set the lights to the passed color
    if (count == 0) {
        mil::System::setLEDs(1, 0, TotalPixels, _red, _green, _blue);
        mil::System::refreshLEDs(1);
    } else {
        mil::System::setLEDs(1, 0, TotalPixels, 0, 0, 0);
    }
	return true;
}
	
int32_t
Flash::loop()
{
	if (_countCompleted >= _count) {
        // If count == 0 we leave the lights on forever
		return _count ? -1 : 0;
	}
	
	uint32_t t = mil::System::millis();
	bool showColor = false;
	bool colorOn = false;
	
	if (_on) {
		if (t > _lastFlash + _duration) {
			colorOn = false;
			showColor = true;
			_lastFlash = t;
			_on = false;
			_countCompleted++;
		}
	} else {
		if (t > _lastFlash + _duration) {
			colorOn = true;
			showColor = true;
			_lastFlash = t;
			_on = true;
		}
	}
	
	if (showColor) {
        if (colorOn) {
            mil::System::setLEDs(1, 0, TotalPixels, _red, _green, _blue);
        } else {
            mil::System::setLEDs(1, 0, TotalPixels, 0, 0, 0);
        }
        mil::System::refreshLEDs(1);
	}
	
    return 0;
}
