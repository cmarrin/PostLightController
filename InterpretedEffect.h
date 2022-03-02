/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// InterpretedEffect Class
//
// This class runs the Interpreter

#pragma once

#include "Effect.h"
#include "NativeColor.h"
#include <Clover.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>

class Device : public clvr::Interpreter
{
public:
	Device(clvr::NativeModule** mod, uint32_t modSize, Adafruit_NeoPixel* pixels)
        : Interpreter(mod, modSize)
        , _pixels(pixels)
    { }
	
    virtual uint8_t rom(uint16_t i) const override
    {
        return EEPROM[i];
    }
    
    virtual void log(const char* s) const override
    {
        Serial.print(s);
    }

private:
	Adafruit_NeoPixel* _pixels;
};

class InterpretedEffect : public Effect
{
public:
	InterpretedEffect(clvr::NativeModule** mod, uint32_t modSize, Adafruit_NeoPixel* pixels);
	virtual ~InterpretedEffect() {}
	
	virtual bool init(uint8_t cmd, const uint8_t* buf, uint32_t size) override;
	virtual int32_t loop() override;
	
	Device::Error error() const { return _device.error(); }
				
private:
	Device _device;
};
