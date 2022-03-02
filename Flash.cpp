/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Flash.h"

bool
Flash::init(uint8_t cmd, const uint8_t* buf, uint32_t size)
{
	Effect::init(cmd, buf, size);
	
	_countCompleted = 0;
	
	if (size < 5 || !buf) {
		Serial.println("***** Buffer not passed. Resetting...");
		_color = Color();
		_count = 0;
		_duration = 0;
	} else {
		_color = Color(buf[0], buf[1], buf[2]);
		_count = buf[3];
		_duration = uint32_t(buf[4]) * 100; // Incoming duration is in 100ms units
		_lastFlash = millis();
		
		Serial.print(F("Flash: color=0x"));
		Serial.print(_color.rgb(), HEX);
		Serial.print(F(" ("));
		Serial.print(_buf[0]);
		Serial.print(F(", "));
		Serial.print(_buf[1]);
		Serial.print(F(", "));
		Serial.print(_buf[2]);
		Serial.print(F("), count="));
		Serial.print(_count);
		Serial.print(F(", duration="));
		Serial.println(_duration);
		
		// If we will be flashing (count != 0) then start with the lights off.
		// Otherwise set the lights to the passed color
		setAllLights(_count ? 0 : _color.rgb());
	}
	return true;
}
	
int32_t
Flash::loop()
{
	if (_countCompleted >= _count) {
		return -1;
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
			color = _color.rgb();
			showColor = true;
			_lastFlash = t;
			_on = true;
		}
	}
	
	if (showColor) {
		setAllLights(color);
	}
	
    return 0;
}

void
Flash::setAllLights(uint32_t color)
{
    for (uint32_t i = 0; i < _pixels->numPixels(); i++) {
        _pixels->setPixelColor(i, color);
        _pixels->show();
    }
}
