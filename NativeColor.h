/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// NativeColor
//
// static color specific methods for Interpreter

#pragma once

#include "Color.h"
#include "Interpreter.h"

namespace clvr {

constexpr uint8_t ColorPrefix = 0x20;

class NativeColor : public NativeModule
{
public:
    using SetLight = void(uint8_t i, uint32_t rgb);
    
    enum class Id {
        LoadColorParam  = ColorPrefix | 0x00,
        SetAllLights    = ColorPrefix | 0x01,
        SetLight        = ColorPrefix | 0x02,
        LoadColorComp   = ColorPrefix | 0x03,
        StoreColorComp  = ColorPrefix | 0x04,
    };
    
    NativeColor(SetLight* setLightFun, uint8_t numPixels) : _setLightFun(setLightFun) , _numPixels(numPixels) { }
    
    virtual bool hasId(uint8_t id) const override;
    virtual uint8_t numParams(uint8_t id) const override;
    virtual int32_t call(Interpreter*, uint8_t id) override;

#ifndef ARDUINO
    virtual void addFunctions(CompileEngine*) override;
#endif

private:
    void setAllLights(uint8_t i);
    
    Color _c[4];
    SetLight* _setLightFun = nullptr;
    uint8_t _numPixels = 0;
};

}
