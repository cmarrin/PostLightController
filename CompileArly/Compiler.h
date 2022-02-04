//
//  Compiler.h
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//
// Arly compiler
//
// Compile Arly statements into a binary stream which can be loaded and executed
//

#pragma once

#include "Opcodes.h"
#include "Scanner.h"
#include <cstdint>
#include <istream>
#include <vector>

namespace arly {

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

struct OpData
{
    OpData() { }
    OpData(std::string str, Op op, OpParams par) : _str(str), _op(op), _par(par) { }
    std::string _str;
    Op _op = Op::Return;
    OpParams _par = OpParams::None;
};

class Compiler
{
public:
    enum class Error {
        None,
        UnrecognizedLanguage,
        ExpectedToken,
        ExpectedType,
        ExpectedValue,
        ExpectedInt,
        ExpectedOpcode,
        ExpectedEnd,
        ExpectedIdentifier,
        ExpectedCommandId,
        ExpectedRegister,
        ExpectedExpr,
        ExpectedArgList,
        ExpectedFormalParams,
        InvalidParamCount,
        UndefinedIdentifier,
        ParamOutOfRange,
        ForEachTooBig,
        IfTooBig,
        ElseTooBig,
        TooManyConstants,
        TooManyVars,
        DefOutOfRange,
        ExpectedDef,
        NoMoreTemps,
        TempNotAllocated,
        TempSizeMismatch,
        StackTooBig,
    };
    
    Compiler() { }
    
    enum class Language { Arly, Clover };
    
    bool compile(std::istream*, Language, std::vector<uint8_t>& executable);

    Error error() const { return _error; }
    Token expectedToken() const { return _expectedToken; }
    const std::string& expectedString() const { return _expectedString; }
    uint32_t lineno() const { return _lineno; }

private:
    Error _error = Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
    uint32_t _lineno;
};

}
