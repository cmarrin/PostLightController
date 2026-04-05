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
Flash::init(uint8_t r, uint8_t g, uint8_t b, uint8_t count, uint16_t duration)
{
    System::initLED(1, PixelPin, TotalPixels);
    
	_countCompleted = 0;
    _red = r;
    _green = g;
    _blue = b;
    _count = count;
    _duration = uint16_t(duration) * 100;
    _lastFlash = System::millis();

    // If we will be flashing (count != 0) then start with the lights off.
    // Otherwise set the lights to the passed color
    if (count == 0) {
        System::setAllLEDs(1, TotalPixels, r, g, b);
    } else {
        System::setAllLEDs(1, TotalPixels, 0, 0, 0);
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
	
	uint32_t t = System::millis();
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
            System::setAllLEDs(1, TotalPixels, _red, _green, _blue);
        } else {
            System::setAllLEDs(1, TotalPixels, 0, 0, 0);
        }
        System::refreshLEDs(1);
	}
	
    return 0;
}
