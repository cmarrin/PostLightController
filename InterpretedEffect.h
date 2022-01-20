// Copyright Chris Marrin 2021
//
// InterpretedEffect Class
//
// This class runs the Interpreter

#pragma once

#include "Effect.h"
#include "Interpreter.h"
#include <EEPROM.h>

class Device : public arly::Interpreter
{
public:
	Device(Adafruit_NeoPixel* pixels) : _pixels(pixels) { }
	
    virtual uint8_t rom(uint16_t i) const override
    {
        return EEPROM[i];
    }
    
    virtual void setLight(uint8_t i, uint32_t rgb) override
    {
        _pixels->setPixelColor(i, rgb);
		_pixels->show();
    }
    
    virtual uint8_t numPixels() const override
    {
        return _pixels->numPixels();
    }

    void logAddr(uint16_t addr) const
	{
		Serial.print("[");
		Serial.print(addr);
		Serial.print("]");
	}
    
    virtual void log(uint16_t addr, uint8_t r, int32_t v) const override
    {
        logAddr(addr);
		Serial.print(": r[");
		Serial.print(r);
		Serial.print("] = ");
		Serial.println(v);
    }
    virtual void logFloat(uint16_t addr, uint8_t r, float v) const override
    {
        logAddr(addr);
		Serial.print(": r[");
		Serial.print(r);
		Serial.print("] = ");
		Serial.println(v);
    }

    virtual void logColor(uint16_t addr, uint8_t r, const Color& c) const override
    {
        logAddr(addr);
        Serial.print(": c[");
		Serial.print(r);
		Serial.print("] = (");
		Serial.print(c.hue());
		Serial.print(", ");
		Serial.print(c.sat());
		Serial.print(", ");
		Serial.print(c.val());
		Serial.println(")");
    }

private:
	Adafruit_NeoPixel* _pixels;
};

class InterpretedEffect : public Effect
{
public:
	InterpretedEffect(Adafruit_NeoPixel* pixels);
	virtual ~InterpretedEffect() {}
	
	virtual bool init(uint8_t cmd, const uint8_t* buf, uint32_t size) override;
	virtual int32_t loop() override;
				
private:
	Device _device;
};
