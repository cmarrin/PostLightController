//
//  Compiler.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Interpreter.h"

using namespace arly;

bool Interpreter::init(uint8_t cmd, const uint8_t* buf, uint8_t size)
{
    _params = buf;
    _numParams = size;
    return _error == Error::None;
}

bool Interpreter::loop()
{
    return _error == Error::None;
}
