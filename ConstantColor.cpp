// Copyright Chris Marrin 2021
//
// ContantColor Class

#include "ConstantColor.h"

ConstantColor::ConstantColor(Adafruit_NeoPixel* pixels)
	: Effect(pixels)
{
}

ConstantColor::~ConstantColor()
{
}

void
ConstantColor::init(const uint8_t* buf, uint32_t size)
{
	Effect::init(buf, size);
	Serial.print("ConstantColor started: hue=");
	Serial.print(_hue);
	Serial.print(", sat=");
	Serial.print(_sat);
	Serial.print(", val=");
	Serial.println(_val);
}

int32_t
ConstantColor::loop()
{
    for (uint32_t i = 0; i < _pixels->numPixels(); i++) {
        _pixels->setPixelColor(i, HSVToRGB(_hue, _sat, _val));
        _pixels->show();
    }

    return 0;
}

void
ConstantColor::setIndexedColor(uint8_t index)
{
	uint32_t color = colorIndexToRGB(index);
    for (uint32_t i = 0; i < _pixels->numPixels(); i++) {
        _pixels->setPixelColor(i, color);
        _pixels->show();
    }
}
