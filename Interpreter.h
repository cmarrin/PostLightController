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
#else
    static inline int32_t random(int32_t min, int32_t max)
    {
        // FIXME: Making lots of assumptions here. Do checking
        int r = rand() % (max - min);
        return r + min;
    }
    
    template<class T> 
    const T& min(const T& a, const T& b)
    {
        return (b < a) ? b : a;
    }

    template<class T> 
    const T& max(const T& a, const T& b)
    {
        return (b > a) ? b : a;
    }
#endif

namespace arly {

class Interpreter
{
public:
    enum class Error {
        None,
        CmdNotFound,
        NestedForEachNotAllowed,
        UnexpectedOpInIf,
		InvalidOp,
        OnlyMemAddressesAllowed,      
    };
    
    Interpreter() { }
    
    bool init(uint8_t cmd, const uint8_t* buf, uint8_t size);
    int32_t loop();

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
    virtual void setLight(uint8_t i, uint32_t rgb) = 0;
    virtual uint8_t numPixels() const = 0;

private:
    int32_t execute(uint16_t addr);
    
    // Index is in bytes
    uint8_t getUInt8ROM(uint16_t index)
    {
        return rom(index);
    }
    
    uint16_t getUInt16ROM(uint16_t index)
    {
        // Little endian
        return uint32_t(getUInt8ROM(index)) | 
               (uint32_t(getUInt8ROM(index + 1)) << 8);
    }

    uint8_t getId() { return getUInt8ROM(_pc++); }
    uint8_t getSz() { return getUInt8ROM(_pc++); }

    void getI(uint8_t& i)
    {
        uint8_t b = getUInt8ROM(_pc++);
        i = b & 0x0f;
    }

    void getRdRs(uint8_t& rd, uint8_t& rs)
    {
        uint8_t b = getUInt8ROM(_pc++);
        rd = b >> 6;
        rs = (b >> 4) & 0x03;
    }

    void getRdRsI(uint8_t& rd, uint8_t& rs, uint8_t& i)
    {
        uint8_t b = getUInt8ROM(_pc++);
        rd = b >> 6;
        rs = (b >> 4) & 0x03;
        i = b & 0x0f;
    }
    
    float getFloat(uint8_t id, uint8_t index)
    {
        if (id >= 0x80) {
            // RAM address
            return intToFloat(_ram[id - 0x80 + index]);
        }
        
        // ROM address
        uint16_t addr = ((id + index) * 4) + _constOffset;

        // Little endian
        uint32_t u = uint32_t(getUInt8ROM(addr)) | 
                    (uint32_t(getUInt8ROM(addr + 1)) << 8) | 
                    (uint32_t(getUInt8ROM(addr + 1)) << 16) | 
                    (uint32_t(getUInt8ROM(addr + 1)) << 24);

        float f;
        memcpy(&f, &u, sizeof(float));
        return f;
    }
    
    uint32_t getInt(uint8_t id, uint8_t index)
    {
        if (id >= 0x80) {
            // RAM address
            return _ram[id - 0x80 + index];
        }
        
        // ROM address
        uint16_t addr = ((id + index) * 4) + _constOffset;

        // Little endian
        uint32_t u = uint32_t(getUInt8ROM(addr)) | 
                    (uint32_t(getUInt8ROM(addr + 1)) << 8) | 
                    (uint32_t(getUInt8ROM(addr + 1)) << 16) | 
                    (uint32_t(getUInt8ROM(addr + 1)) << 24);

        return u;
    }
    
    void storeInt(uint8_t id, uint8_t index, uint32_t v)
    {
        // Only RAM
        if (id < 0x80) {
            return;
        }
        _ram[id - 0x80 + index] = v;
    }

    void storeColor(uint8_t id, uint8_t index, uint8_t creg)
    {
        if (id < 0x80) {
            return;
        }
        
        index = + (id - 0x80);
        
        const Color& c = _c[creg];
        _ram[index] = floatToInt(c.hue());
        _ram[index + 1] = floatToInt(c.sat());
        _ram[index + 2] = floatToInt(c.val());
    }

    float intToFloat(uint32_t i)
    {
        float f;
        memcpy(&f, &i, sizeof(float));
        return f;
    }

    uint32_t floatToInt(float f)
    {
        uint32_t i;
        memcpy(&i, &f, sizeof(float));
        return i;
    }
    
    Error _error = Error::None;
    const uint8_t* _params = nullptr;
    uint8_t _paramsSize = 0;
    
    uint32_t _ram[64];
    uint32_t _v[4];
    Color _c[4];
    
    uint16_t _pc = 0;
    uint16_t _constOffset = 0; // In bytes
    uint8_t _numParams = 0;
    uint16_t _initStart = 0;
    uint16_t _loopStart = 0;
    
    int16_t _foreachSz = -1; // -1 means no foreach is active
    uint8_t _foreachIReg = 0;
    uint32_t _foreachCount = 0;
    uint16_t _foreachLoopAddr = 0;
};

}
