/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Flash.h"

#include "Defines.h"
#include "PostLightController.h"

bool
Flash::init(mil::NeoPixel* pixels, uint8_t h, uint8_t s, uint8_t v, uint8_t count, uint16_t duration)
{
	_countCompleted = 0;
	
    _color = pixels->color(h, s, v);
    _count = count;
    _duration = uint16_t(duration) * 100;
    _lastFlash = millis();

    // If we will be flashing (count != 0) then start with the lights off.
    // Otherwise set the lights to the passed color
    pixels->setLights(0, TotalPixels - 1, _count ? 0 : pixels->color(h, s, v));
    pixels->show();

	return true;
}
	
int32_t
Flash::loop(mil::NeoPixel* pixels)
{
	if (_countCompleted >= _count) {
        // If count == 0 we leave the lights on forever
		return _count ? -1 : 0;
	}
	
	uint32_t t = millis();
	bool showColor = false;
	uint32_t color = 0;
	
	if (_on) {
		if (t > _lastFlash + _duration) {
			color = 0;
			showColor = true;
			_lastFlash = t;
			_on = false;
			_countCompleted++;
		}
	} else {
		if (t > _lastFlash + _duration) {
			color = _color;
			showColor = true;
			_lastFlash = t;
			_on = true;
		}
	}
	
	if (showColor) {
		pixels->setLights(0, TotalPixels - 1, color);
        pixels->show();
	}
	
    return 0;
}
