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

#include "Interpreter.h"
#include "NeoPixel.h"

static constexpr uint32_t StackSize = 1024;

class MyInterpreter : public clvr::Interpreter<StackSize>
{
};

class InterpretedEffect
{
public:
	InterpretedEffect(mil::NeoPixel* pixels) : _pixels(pixels) { }
	
	bool init(uint8_t cmd, const uint8_t* buf, uint32_t size);
	int32_t loop();
	
	clvr::Memory::Error error() const { return _interp.error(); }
    int16_t errorAddr() const { return _interp.errorAddr(); }

    uint8_t* stackBase() { return &(_interp.memMgr()->stack().getAbs(0)); }

private:
    static void setLight(uint16_t id, clvr::InterpreterBase*, void* data);
	MyInterpreter _interp;
    mil::NeoPixel* _pixels;
};
