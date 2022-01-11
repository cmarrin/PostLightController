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

	if (size < 3 || !buf) {
		Serial.println("***** Buffer not passed. Resetting...");
		_color = Color();
	} else {
		_color = Color(buf[0], buf[1], buf[2]);
	}

	Serial.print("ConstantColor started: color=0x");
	Serial.println(_color.rgb(), HEX);
}

int32_t
ConstantColor::loop()
{
	setColor(_color);
    return 0;
}

void
ConstantColor::setColor(const Color& color)
{
    for (uint32_t i = 0; i < _pixels->numPixels(); i++) {
        _pixels->setPixelColor(i, color.rgb());
    }
    _pixels->show();
}
