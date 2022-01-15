//
//  Decompiler.h
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//
// Arly compiler
//
// Decompile Arly executable into source string
//

#pragma once

#include "Compiler.h"

namespace arly {

class Decompiler
{
public:
    enum class Error {
        None,
        InvalidSignature,
        InvalidOp,
    };
    
    Decompiler(const std::vector<uint8_t>* in, std::string* out)
        : _in(in)
        , _out(out)
    {
        _it = _in->begin();
    }
    
    bool decompile();
    
    Error error() const { return _error; }

private:
    void constants();
    void effects();
    void init();
    void loop();
    Op statement();
    
    uint32_t getUInt32()
    {
        // Little endian
        return uint32_t(*_it++) | (uint32_t(*_it++) << 8) | 
                (uint32_t(*_it++) << 16) | (uint32_t(*_it++) << 24);
    }
    
    uint16_t getUInt16()
    {
        // Little endian
        return uint32_t(*_it++) | (uint32_t(*_it++) << 8);
    }
    
    uint16_t getUInt8() { return *_it++; }
    
    std::string regString(uint8_t r, bool isColor = false);
    
    void doIndent()
    {
        for (uint32_t i = 0; i < _indent; ++i) {
            _out->append("    ");
        }
    }
    
    void decIndent()
    {
        if (_indent == 0) {
            _out->append("*** Error, tried to indent past 0!!!\n");
        } else {
            _indent--;
        }
    }

    void incIndent()
    {
        _indent++;
    }

    Error _error = Error::None;
    const std::vector<uint8_t>* _in;
    std::vector<uint8_t>::const_iterator _it;
    std::string* _out;
    int32_t _indent = 0;
};

}
