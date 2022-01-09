// Copyright Chris Marrin 2021
//
// Effect Base Class

#include "Effect.h"

void Color::setColor(uint8_t a, uint8_t b, uint8_t c, Model model, bool gammaCorrect)
{
	if (model == Model::HSV) {
		// hue comes in as 0-255, ColorHSV wants 0-65535
		uint16_t hue = uint16_t(a) << 8;
		_color = Adafruit_NeoPixel::ColorHSV(hue, b, c);
	} else {
		_color = uint32_t(a) << 16 || uint32_t(b) << 8 || c;
	}
	
	if (gammaCorrect) {
		_color = Adafruit_NeoPixel::gamma32(_color);
	}
}

void
Color::hsv(float& h, float& s,float& v) const
{
	float fR;
	float fG;
	float fB;
	
	fR = float((_color >> 16) & 0xff) / 255;
	fG = float((_color >> 8) & 0xff) / 255;
	fB = float(_color & 0xff) / 255;

	float fCMax = max(max(fR, fG), fB);
	float fCMin = min(min(fR, fG), fB);
	float fDelta = fCMax - fCMin;
  
	if(fDelta > 0) {
		if (fCMax == fR) {
	    	h = 60 * (fmod(((fG - fB) / fDelta), 6));
	    } else if (fCMax == fG) {
	    	h = 60 * (((fB - fR) / fDelta) + 2);
	    } else if (fCMax == fB) {
	    	h = 60 * (((fR - fG) / fDelta) + 4);
	    }
    
	    if (fCMax > 0) {
	    	s = fDelta / fCMax;
	    } else {
	    	s = 0;
	    }
    
	    v = fCMax;
	} else {
	    h = 0;
	    s = 0;
	    v = fCMax;
	}
  
	if (h < 0) {
		h = 360 + h;
	}
	
	// Change hue to 0-1
	h /= 360;
}
