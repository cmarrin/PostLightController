//
//  Compiler.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Interpreter.h"

using namespace arly;

bool
Interpreter::init(uint8_t cmd, const uint8_t* buf, uint8_t size)
{
    _params = buf;
    _paramsSize = size;
    
    _constOffset = 8;
    
    // Find command
    uint32_t constSize = uint32_t(getUInt8ROM(4)) * 4;
    for (uint32_t i = _constOffset + constSize; ; ) {
        uint8_t c = rom(i);
        if (c == 0) {
            _error = Error::CmdNotFound;
            return false;
        }
        if (c == cmd) {
            // found cmd
            _numParams = getUInt8ROM(i + 1);
            _initStart = getUInt16ROM(i + 2);
            _loopStart = getUInt16ROM(i + 4);
            break;
        }
        
        i += 6;
    }
    
    // Execute init();
    execute(_initStart);
    
    return _error == Error::None;
}

bool
Interpreter::loop()
{
    execute(_loopStart);
    return _error == Error::None;
}

void
Interpreter::execute(uint16_t addr)
{
    _pc = addr;
}
