//
//  Compiler.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Decompiler.h"

#include "CompileEngine.h"

using namespace arly;

bool Decompiler::decompile()
{    
    // Make sure we start with 'arly'
    if (getUInt8() != 'a' || getUInt8() != 'r' || getUInt8() != 'l' || getUInt8() != 'y') {
        _error = Error::InvalidSignature;
        return false;
    }
    
    try {
        constants();
        effects();
    }
    catch(...) {
        return false;
    }
    
    return true;
}

void
Decompiler::constants()
{
    _out->append("const\n");
    
    uint8_t size = *_it++;
    _it += 3;
    
    for (uint8_t i = 0; i < size; ++i) {
        _out->append("[");
        _out->append(std::to_string(i));
        _out->append("] = ");
        _out->append(std::to_string(getUInt32()));
        _out->append("\n");
    }
    _out->append("\n");
}

void
Decompiler::effects()
{
    struct Entry
    {
        Entry(uint8_t cmd, uint8_t params, uint16_t init, uint16_t loop)
            : _cmd(cmd)
            , _params(params)
            , _init(init)
            , _loop(loop)
        { }
        
        uint8_t _cmd, _params;
        uint16_t _init, _loop;
    };
    
    std::vector<Entry> entries;
    
    while(1) {
        uint8_t cmd = getUInt8();
        if (!cmd) {
            break;
        }
        entries.emplace_back(cmd, getUInt8(), getUInt16(), getUInt16());
    }
    
    for (auto& entry : entries) {
        _out->append("effect '");
        char c[2] = " ";
        c[0] = entry._cmd;
        _out->append(c);
        _out->append("' ");
        _out->append(std::to_string(entry._params));
        _out->append("\n");
        
        init();
        loop();
    }
}

void
Decompiler::init()
{
    _out->append("init\n");
    
    while(1) {
        if (statement() == Op::End) {
            break;
        }
    }
    _out->append("\n");
}

void
Decompiler::loop()
{
    _out->append("loop\n");
    
    while(1) {
        if (statement() == Op::End) {
            break;
        }
    }
    _out->append("\n");
}

std::string
Decompiler::regString(uint8_t r, bool isColor)
{
    switch(r) {
        case 0: return isColor ? "c0" : "r0";
        case 1: return isColor ? "c1" : "r1";
        case 2: return isColor ? "c2" : "r2";
        case 3: return isColor ? "c3" : "r3";
        default: return "*** ERR ***";
    }
}

Op
Decompiler::statement()
{
    uint8_t opInt = getUInt8();
    if (Op(opInt) == Op::End) {
        return Op::End;
    }
    
    uint8_t r = 0;
    
    if (opInt >= 0x80) {
        // Get r from lowest 2 bits
        r = opInt & 0x03;
        opInt &= 0xfc;
    }
    
    OpData opData;
    if (!CompileEngine::opDataFromOp(Op(opInt), opData)) {
        _error = Error::InvalidOp;
        throw true;
    }

    _out->append(opData._str);
    _out->append(" ");
    
    // Get params
    switch(opData._par) {
        case OpParams::None:
            break;
        case OpParams::R:
            _out->append(regString(r));
            break;
        case OpParams::C:
            _out->append(regString(r, true));
            break;
        case OpParams::R_I:
            _out->append(regString(r));
            _out->append(" ");
            _out->append(std::to_string(getUInt8()));
            break;
        case OpParams::C_I:
            _out->append(regString(r, true));
            _out->append(" ");
            _out->append(std::to_string(getUInt8()));
            break;
        case OpParams::R_Id:
            _out->append(regString(r));
            _out->append(" [");
            _out->append(std::to_string(getUInt8()));
            _out->append("]");
            break;
        case OpParams::C_Id:
            _out->append(regString(r, true));
            _out->append(" [");
            _out->append(std::to_string(getUInt8()));
            _out->append("]");
            break;
        case OpParams::Id_R:
            _out->append("[");
            _out->append(std::to_string(getUInt8()));
            _out->append("] ");
            _out->append(regString(r));
            break;
        case OpParams::Id_C:
            _out->append("[");
            _out->append(std::to_string(getUInt8()));
            _out->append("] ");
            _out->append(regString(r, true));
            break;
        case OpParams::Rd_Id_Rs:
            break;
        case OpParams::Cd_Id_Rs:
            break;
        case OpParams::Rd_Id_Rs_I:
            break;
        case OpParams::Id_Rd_Rs:
            break;
        case OpParams::Id_Rd_Cs:
            break;
        case OpParams::Id_Rd_I_Rs:
            break;
        case OpParams::Rd_Rs:
            break;
        case OpParams::Cd_Rs:
            break;
        case OpParams::Rd_Cs:
            break;
        case OpParams::Cd_Cs:
            break;
        case OpParams::Id:
            _out->append("[");
            _out->append(std::to_string(getUInt8()));
            _out->append("]");
            break;
    }
    
    
    
    _out->append("\n");
    return Op(opInt);
}