//
//  Interpreter.h
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//
// Arly Interpreter base class
//
// Interpret Arly executable
//

#pragma once

#include "Compiler.h"

namespace arly {

class Interpreter
{
public:
    enum class Error {
        None,
    };
    
    Interpreter() { }
    
    bool init(uint8_t cmd, const uint8_t* buf, uint8_t size);
    bool loop();

    Error error() const { return _error; }

private:
    Error _error = Error::None;
    const uint8_t* _params = nullptr;
    uint8_t _numParams = 0;
};

}
