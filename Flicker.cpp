// Copyright Chris Marrin 2021
//
// Flicker Class
//
// This class flickers the LEDs according to input params

#include "Flicker.h"

const Flicker::Speed Flicker::_speedTable[ ] = {
    { 250, 255, 40 },
    { 220, 250, 30 },
    { 180, 240, 20 },
    { 150, 200, 20 },
    {  70, 120, 20 },
    {  20,  80, 20 },
    {   8,  40, 10 },
    {   2,   5, 10 },
};

Flicker::Flicker(Adafruit_NeoPixel* pixels)
	: Effect(pixels)
{
	_leds = new LED[_pixels->numPixels()];
}

Flicker::~Flicker()
{
	delete [ ] _leds;
	_leds = nullptr;
}

void
Flicker::init(const uint8_t* buf, uint32_t size)
{
	Effect::init(buf, size);
	
	if (size < 4 || !_buf) {
		Serial.println("***** Flicker: Buffer not passed");
		return;
	}

	if (_buf[3] > 7) {
		Serial.println("***** Flicker: speed out of range");
		return;
	}

	_color = Color(_buf[0], _buf[1], _buf[2]);

	Serial.print("Flicker started: hue=");
	Serial.print(_buf[0]);
	Serial.print(", sat=");
	Serial.print(_buf[1]);
	Serial.print(", val=");
	Serial.print(_buf[2]);
	Serial.print(", speed=");
	Serial.println(_buf[3]);
}

int32_t
Flicker::loop()
{
    for (uint32_t i = 0; i < _pixels->numPixels(); i++) {
        LED& led = _leds[i];
        led.off += led.inc;

        // Check for going past lim (increasing) or below 0 (decreasing)
        if (led.inc > 0) {
            if (led.off >= led.lim) {
                led.off = led.lim;
                led.inc = -led.inc;
            }
        } else if (led.off <= 0) {
            // We are done with the throb, select a new inc and lim
            led.off = 0;

            // Increment inc value for each step, in 1/10s
            led.inc = randomFloat(IncMin, IncMax);

            // Random number of steps to throb up and down
            led.lim = led.inc + randomFloat(_speedTable[_buf[3]].stepsMin, _speedTable[_buf[3]].stepsMax);
        }

        // What is the relative brightness. led.off always starts at 0, so that is 
        // a brightness multiplier of 1. at its limit it is equal to led.lim, so
        // that is a multiplier of 1 + led.lim / 255.
        float brightness = (BrightnessMin + led.off) / 255;
		float val = _color.val() * brightness;
		val = max(val, ValMin);

        _pixels->setPixelColor(i, Color(_color.hue(), _color.sat(), _color.val() * brightness).rgb());
        _pixels->show();
    }

    return _speedTable[min(_buf[3], 9)].delay;
}
