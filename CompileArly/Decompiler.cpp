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
    doIndent();
    incIndent();
    _out->append("const\n");
    
    uint8_t size = *_it++;
    _it += 3;
    
    for (uint8_t i = 0; i < size; ++i) {
        doIndent();
        _out->append("[");
        _out->append(std::to_string(i));
        _out->append("] = ");
        _out->append(std::to_string(getUInt32()));
        _out->append("\n");
    }
    
    _out->append("\n");
    decIndent();

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
        doIndent();
        incIndent();
        _out->append("effect '");
        char c[2] = " ";
        c[0] = entry._cmd;
        _out->append(c);
        _out->append("' ");
        _out->append(std::to_string(entry._params));
        _out->append("\n");
        
        init();
        loop();
        decIndent();
    }
}

void
Decompiler::init()
{
    doIndent();
    incIndent();
    _out->append("init\n");
    
    while(1) {
        if (statement() == Op::End) {
            break;
        }
    }
    _out->append("\n");
    decIndent();
}

void
Decompiler::loop()
{
    doIndent();
    incIndent();
    _out->append("loop\n");
    
    while(1) {
        if (statement() == Op::End) {
            break;
        }
    }
    _out->append("\n");
    decIndent();
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
    
    // There is an Op::EndIf that is at the end of an if statement
    // Handle it separately
    if (Op(opInt) == Op::DummyEnd) {
        decIndent();
        doIndent();
        _out->append("end\n\n");
        return Op::DummyEnd;
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

    // outdent Else one
    if (opData._op == Op::Else) {
        decIndent();
    }

    // Add blank like before if and foreach
    if (opData._op == Op::ForEach || opData._op == Op::If) {
        _out->append("\n");
    }

    doIndent();

    _out->append(opData._str);
    _out->append(" ");
    
    // Get params
    uint8_t id;
    uint8_t rdrsi;
    
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
            id = getUInt8();
            rdrsi = getUInt8();
            
            _out->append(regString(rdrsi >> 6));
            _out->append(" [");
            _out->append(std::to_string(id));
            _out->append("] ");
            _out->append(regString((rdrsi >> 4) & 0x03));
            break;
        case OpParams::Cd_Id_Rs:
            id = getUInt8();
            rdrsi = getUInt8();
            
            _out->append(regString(rdrsi >> 6, true));
            _out->append(" [");
            _out->append(std::to_string(id));
            _out->append("] ");
            _out->append(regString((rdrsi >> 4) & 0x03));
            break;
        case OpParams::Rd_Id_Rs_I:
            id = getUInt8();
            rdrsi = getUInt8();
            
            _out->append(regString(rdrsi >> 6));
            _out->append(" [");
            _out->append(std::to_string(id));
            _out->append("] ");
            _out->append(regString((rdrsi >> 4) & 0x03));
            _out->append(" ");
            _out->append(std::to_string(rdrsi & 0x0f));
            break;
        case OpParams::Id_Rd_Rs:
            id = getUInt8();
            rdrsi = getUInt8();
            
            _out->append("[");
            _out->append(std::to_string(id));
            _out->append("] ");
            _out->append(regString(rdrsi >> 6, true));
            _out->append(" ");
            _out->append(regString((rdrsi >> 4) & 0x03));
            break;
        case OpParams::Id_Rd_Cs:
            id = getUInt8();
            rdrsi = getUInt8();
            
            _out->append("[");
            _out->append(std::to_string(id));
            _out->append("] ");
            _out->append(regString(rdrsi >> 6));
            _out->append(" ");
            _out->append(regString((rdrsi >> 4) & 0x03, true));
            break;
        case OpParams::Id_Rd_I_Rs:
            id = getUInt8();
            rdrsi = getUInt8();
            
            _out->append("[");
            _out->append(std::to_string(id));
            _out->append("] ");
            _out->append(regString(rdrsi >> 6));
            _out->append(" ");
            _out->append(std::to_string(rdrsi & 0x0f));
            _out->append(" ");
            _out->append(regString((rdrsi >> 4) & 0x03));
             break;
        case OpParams::Rd_Rs:
            rdrsi = getUInt8();
            _out->append(regString(rdrsi >> 6));
            _out->append(" ");
            _out->append(regString((rdrsi >> 4) & 0x03));
            break;
        case OpParams::Cd_Rs:
            rdrsi = getUInt8();
            _out->append(regString(rdrsi >> 6, true));
            _out->append(" ");
            _out->append(regString((rdrsi >> 4) & 0x03));
            break;
        case OpParams::Rd_Cs:
            rdrsi = getUInt8();
            _out->append(regString(rdrsi >> 6));
            _out->append(" ");
            _out->append(regString((rdrsi >> 4) & 0x03, true));
            break;
        case OpParams::Cd_Cs:
            rdrsi = getUInt8();
            _out->append(regString(rdrsi >> 6, true));
            _out->append(" ");
            _out->append(regString((rdrsi >> 4) & 0x03, true));
            break;
        case OpParams::Id:
            _out->append("[");
            _out->append(std::to_string(getUInt8()));
            _out->append("]");
            break;
        case OpParams::R_Sz:
            _out->append(regString(r));
            _out->append(" [");
            _out->append(std::to_string(getUInt8()));
            _out->append("]");
            break;
        case OpParams::Sz:
            _out->append("[");
            _out->append(std::to_string(getUInt8()));
            _out->append("]");
            break;
    }
    
    _out->append("\n");
    
    if (opData._op == Op::ForEach || opData._op == Op::If || opData._op == Op::Else) {
        incIndent();
    }
    return Op(opInt);
}
