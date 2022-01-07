// Copyright Chris Marrin 2021
//
// ContantColor Class

#include "Flash.h"

int32_t
Flash::loop()
{
	if (_count >= _param1) {
		return -1;
	}
	
	uint32_t t = millis();
	bool showColor = false;
	uint32_t color = 0;
	uint32_t duration = _param2 * 100;
	
	if (_on) {
		if (t > _lastFlash + duration) {
			color = HSVToRGB(0, 0, 0);
			showColor = true;
			_lastFlash = t;
			_on = false;
			_count++;
		}
	} else {
		if (t > _lastFlash + duration) {
			color = HSVToRGB(_hue, _sat, _val);
			showColor = true;
			_lastFlash = t;
			_on = true;
		}
	}
	
	if (showColor) {
	    for (uint32_t i = 0; i < _pixels->numPixels(); i++) {
	        _pixels->setPixelColor(i, color);
	        _pixels->show();
	    }
	}
	
    return 0;
}
