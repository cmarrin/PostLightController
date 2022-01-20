// Copyright Chris Marrin 2021
//
// ContantColor Class

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
		
		Serial.print("Flash: color=0x");
		Serial.print(_color.rgb(), HEX);
		Serial.print(", count=");
		Serial.print(_count);
		Serial.print(", duration=");
		Serial.println(_duration);
		
		// Start with the lights off
		setAllLights(0);
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
