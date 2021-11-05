// Copyright Chris Marrin 2021
//
// Flicker Class
//
// This class flickers the LEDs according to input params

#include "Flicker.h"

static const Flicker::Speed Flicker::_speedTable[] = {
    { 300, 350, 40 },
    { 250, 280, 30 },
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
Flicker::init(uint8_t param1, uint8_t param2, uint8_t param3)
{
	Effect::init(param1, param2, param3);
	
	_speed = param3 & 7;
	
    _stepsMin = _speedTable[min(_speed, 9)].stepsMin;
    _stepsMin = _speedTable[min(_speed, 9)].stepsMax;

    // A val of 0 will be set to minVal / 255, a val of 7 is set to 1
    // values between are evenly spaced
	_val *= 255;
    _val = _val * float(255 - _minVal) / 255 + _minVal;
    _val /= 255;
}

uint32_t
Flicker::loop()
{
    for (int i = 0; i < _pixels->numPixels(); i++) {
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

            // Increment in value for each step, in 1/10s
            led.inc = float(random(_incMin * 100, _incMax * 100)) / 100;

            // Random number of steps to throb up and down
            led.lim = led.inc + random(_stepsMin, _stepsMax);
        }

        // What is the relative brightness. led.off always starts at 0, so that is 
        // a brightness multiplier of 1. at its limit it is equal to led.lim, so
        // that is a multiplier of 1 + led.lim / 255.
        float brightness = (_brightnessMin + led.off) / 255;
        _pixels->setPixelColor(i, HSVToRGB(_hue, _sat, _val * brightness));
        _pixels->show();
    }

    return _speedTable[min(_speed, 9)].delay;
}
