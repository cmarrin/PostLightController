// Copyright Chris Marrin 2021
//
// Effect Base Class

#include "Effect.h"

void Color::setColor(uint8_t a, uint8_t b, uint8_t c, bool gammaCorrect)
{
	_gammaCorrect = gammaCorrect;

	_hue = float(a) / 255;
	_sat = float(b) / 255;
	_val = float(c) / 255;
}
