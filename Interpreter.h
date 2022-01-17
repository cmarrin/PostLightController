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

#include "Color.h"

#include <string.h>

#ifdef ARDUINO
#include <ArduinoSTL.h>
#else
    static inline int32_t random(int32_t min, int32_t max)
    {
        // FIXME: Making lots of assumptions here. Do checking
        int r = rand() % (max - min);
        return r + min;
    }
#endif

namespace arly {

class Interpreter
{
public:
    enum class Error {
        None,
        CmdNotFound,
    };
    
    Interpreter() { }
    
    bool init(uint8_t cmd, const uint8_t* buf, uint8_t size);
    bool loop();

    Error error() const { return _error; }
    
	// Return a float with a random number between min and max.
	// Multiply min and max by 100 and then divide the result 
	// to give 2 decimal digits of precision
	static float randomFloat(uint8_t min, uint8_t max)
	{
		return float(random(int32_t(min) * 100, int32_t(max) * 100)) / 100;
	}
	
protected:
    virtual uint8_t rom(uint16_t i) const = 0;

private:
    void execute(uint16_t addr);
    
    // Index is in bytes
    uint8_t getUInt8ROM(uint16_t index)
    {
        return rom(index);
    }
    
    // Index is in 4 byte elements
    uint32_t getUInt32Const(uint16_t index)
    {
        index = (index * 4) + _constOffset;

        // Little endian
        return uint32_t(getUInt8ROM(index)) | 
               (uint32_t(getUInt8ROM(index + 1)) << 8) | 
               (uint32_t(getUInt8ROM(index + 1)) << 16) | 
               (uint32_t(getUInt8ROM(index + 1)) << 24);
    }
    
    float getFloatConst(uint16_t index)
    {
        uint32_t u = getUInt32Const(index);
        float f;
        memcpy(&f, &u, sizeof(float));
        return f;
    }
    
    uint16_t getUInt16ROM(uint16_t index)
    {
        // Little endian
        return uint32_t(getUInt8ROM(index)) | 
               (uint32_t(getUInt8ROM(index + 1)) << 8);
    }
        
    Error _error = Error::None;
    const uint8_t* _params = nullptr;
    uint8_t _paramsSize = 0;
    
    uint32_t _ram[64];
    uint32_t _r[4];
    Color _c[4];
    
    uint16_t _pc = 0;
    uint16_t _constOffset = 0; // In bytes
    uint8_t _numParams = 0;
    uint16_t _initStart = 0;
    uint16_t _loopStart = 0;
};

}
