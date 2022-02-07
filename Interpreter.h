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
#include "Opcodes.h"

#include <string.h>

#ifdef ARDUINO
#else
    static inline int32_t random(int32_t min, int32_t max)
    {
        if (min >= max) {
            return max;
        }
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

static constexpr uint8_t MaxStackSize = 128;    // Could be 255 but let's avoid excessive 
                                                // memory usage
static constexpr uint8_t StackOverhead = 64;    // Amount added to var mem high water mark
                                                // for stack growth.
static constexpr uint8_t MaxTempSize = 32;      // Allocator uses a uint32_t map. That would 
                                                // need to be changed to increase this.
static constexpr uint8_t ParamsSize = 16;       // Constrained by the 4 bit field with the index

static inline float intToFloat(uint32_t i)
{
    float f;
    memcpy(&f, &i, sizeof(float));
    return f;
}

static inline uint32_t floatToInt(float f)
{
    uint32_t i;
    memcpy(&i, &f, sizeof(float));
    return i;
}

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
        StackOverrun,
        AddressOutOfRange,
        InvalidColorComp,
        ExpectedSetFrame,
        InvalidNativeFunction,
        MisalignedStack,
    };
    
    enum class NativeFunction {
        None = 0,
        LoadColorParam = 1,
        SetAllLights = 2,
    };
    
    Interpreter() { }
    
    bool init(uint8_t cmd, const uint8_t* buf, uint8_t size);
    int32_t loop();

    Error error() const { return _error; }
    
    // Returns -1 if error was not at any pc addr
    int16_t errorAddr() const { return _errorAddr; }
    
	// Return a float with a random number between min and max.
	// The random function takes ints. Multiply inputs by 1000 then divide the result
    // by the same to get a floating point result. That effectively makes the range
    // +/-2,000,000.
	static float random(float min, float max)
	{
		return float(::random(int32_t(min * 1000), int32_t(max * 1000))) / 1000;
	}
	
	static int32_t random(int32_t min, int32_t max)
	{
		return ::random(min, max);
	}
	
protected:
    virtual uint8_t rom(uint16_t i) const = 0;
    virtual void setLight(uint8_t i, uint32_t rgb) = 0;
    virtual uint8_t numPixels() const = 0;
    virtual void log(uint16_t addr, uint8_t r, int32_t v) const = 0;
    virtual void logFloat(uint16_t addr, uint8_t r, float v) const = 0;
    virtual void logColor(uint16_t addr, uint8_t r, const Color& c) const = 0;

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
    uint8_t getConst() { return getUInt8ROM(_pc++); }
    uint8_t getSz() { return getUInt8ROM(_pc++); }
    void getPL(uint8_t& p, uint8_t& l)
    {
        uint8_t pl = getUInt8ROM(_pc++);
        p = pl >> 4;
        l = pl & 0x0f;
    }

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
    
    float loadFloat(uint8_t id, uint8_t index = 0)
    {
        uint32_t i = loadInt(id, index);
        float f;
        memcpy(&f, &i, sizeof(float));
        return f;
    }
    
    void storeFloat(uint8_t id, float v) { storeFloat(id, 0, v); }
    
    void storeFloat(uint8_t id, uint8_t index, float v) { storeInt(id, index, floatToInt(v)); }
    
    uint32_t loadInt(uint8_t id, uint8_t index = 0)
    {
        if (id < GlobalStart) {
            // ROM address
            uint16_t addr = ((id + index) * 4) + _constOffset;

            // Little endian
            uint32_t u = uint32_t(getUInt8ROM(addr)) | 
                        (uint32_t(getUInt8ROM(addr + 1)) << 8) | 
                        (uint32_t(getUInt8ROM(addr + 2)) << 16) | 
                        (uint32_t(getUInt8ROM(addr + 3)) << 24);

            return u;
        }
        
        if (id < LocalStart) {
            // Global address
            return _global[id - GlobalStart + index];
        }

        // Local address. Relative to current bp.
        return _stack[id - LocalStart + index + _bp];        
    }
    
    void storeInt(uint8_t id, uint32_t v) { storeInt(id, 0, v); }
    
    void storeInt(uint8_t id, uint8_t index, uint32_t v)
    {
        // Only Global or Local
        if (id < GlobalStart) {
            return;
        }

        if (id < LocalStart) {
            // Global address
            uint32_t addr = uint32_t(id) - GlobalStart + uint32_t(index);
            _global[addr] = v;
            return;
        }
            
        // Local address. Relative to current bp.
        uint32_t addr = uint32_t(id) - LocalStart + uint32_t(index) + _bp;
        if (addr >= MaxStackSize) {
            _error = Error::AddressOutOfRange;
            _errorAddr = _pc - 1;
        } else {
            _stack[addr] = v;
        }
    }
    
    bool setFrame(uint8_t params, uint8_t locals);
    bool restoreFrame();
    void setAllLights(uint8_t c);

    int32_t animate(uint32_t index);
    
    Error _error = Error::None;
    int16_t _errorAddr = -1;
    
    uint8_t _params[ParamsSize];
    uint8_t _paramsSize = 0;

    uint32_t _v[4];
    Color _c[4];

    uint32_t* _global = nullptr;
    uint16_t _globalSize = 0;
    uint32_t* _stack = nullptr;
    uint16_t _stackSize = 0;
    
    uint16_t _pc = 0;
    uint16_t _sp = 0;
    uint16_t _bp = 0;
    
    uint16_t _constOffset = 0; // In bytes
    uint8_t _numParams = 0;
    uint16_t _initStart = 0;
    uint16_t _loopStart = 0;
    uint16_t _codeOffset = 0; // Used by Call to go to the correct absolute address
    
    int16_t _foreachSz = -1; // -1 means no foreach is active
    uint8_t _foreachIReg = 0;
    uint32_t _foreachCount = 0;
    uint16_t _foreachLoopAddr = 0;
};

}
