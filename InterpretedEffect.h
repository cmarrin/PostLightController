// Copyright Chris Marrin 2021
//
// InterpretedEffect Class
//
// This class runs the Interpreter

#pragma once

#include "Effect.h"
#include "NativeColor.h"
#include <Clover.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>

class Device : public arly::Interpreter
{
public:
	Device(arly::NativeModule** mod, uint32_t modSize, Adafruit_NeoPixel* pixels)
        : Interpreter(mod, modSize)
        , _pixels(pixels)
    { }
	
    virtual uint8_t rom(uint16_t i) const override
    {
        return EEPROM[i];
    }
    
    void logAddr(uint16_t addr) const
	{
		Serial.print(F("["));
		Serial.print(addr);
		Serial.print(F("]"));
	}
    
    virtual void logInt(uint16_t addr, int8_t, int32_t v) const override
    {
		Serial.print(F("*** LogInt at addr "));
        logAddr(addr);
		Serial.print(F(": "));
		Serial.println(v);
    }
    virtual void logFloat(uint16_t addr, int8_t, float v) const override
    {
		Serial.print(F("*** LogFloat at addr "));
        logAddr(addr);
		Serial.print(F(": "));
		Serial.println(v);
    }

    virtual void logHex(uint16_t addr, int8_t i, uint32_t v) const override
    {
		Serial.print(F("*** LogHex at addr "));
        logAddr(addr);
        Serial.print(F(": v["));
		Serial.print(i);
		Serial.print(F("] = ("));
		Serial.print(v, HEX);
        Serial.println(F(")"));
    }

    virtual void logString(const char* s) const override
    {
        Serial.print(s);
    }

private:
	Adafruit_NeoPixel* _pixels;
};

class InterpretedEffect : public Effect
{
public:
	InterpretedEffect(arly::NativeModule** mod, uint32_t modSize, Adafruit_NeoPixel* pixels);
	virtual ~InterpretedEffect() {}
	
	virtual bool init(uint8_t cmd, const uint8_t* buf, uint32_t size) override;
	virtual int32_t loop() override;
	
	Device::Error error() const { return _device.error(); }
				
private:
	Device _device;
};
