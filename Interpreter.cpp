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
    bool found = false;
    uint16_t index = _constOffset + constSize;
    
    while (1) {
        uint8_t c = rom(index);
        if (c == 0) {
            index++;
            break;
        }
        if (c == cmd) {
            // found cmd
            _numParams = getUInt8ROM(index + 1);
            _initStart = getUInt16ROM(index + 2);
            _loopStart = getUInt16ROM(index + 4);
            
            found = true;
            
            //Need to keep looping through all effects
            // to find where the code starts
        }
        
        index += 6;
    }
    
    if (!found) {
        _error = Error::CmdNotFound;
        return false;
    }

    _initStart += index;
    _loopStart += index;
    
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
