//
//  ArlyCompileEngine.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "ArlyCompileEngine.h"

#include <cmath>
#include <map>

using namespace arly;

std::vector<OpData> CompileEngine::_opcodes = {
    { "LoadI",          Op::LoadI           , OpParams::Rd_Id_Rs_I },
    { "StoreI",         Op::StoreI          , OpParams::Id_Rd_I_Rs },

    { "LoadX",          Op::LoadX           , OpParams::Rd_Id_Rs_I },
    { "LoadIX",         Op::LoadIX          , OpParams::Rd_Rs_I },
    { "StoreIX",        Op::StoreIX         , OpParams::Rd_I_Rs },

    { "MoveColor",      Op::MoveColor       , OpParams::Cd_Cs },
    { "Move",           Op::Move            , OpParams::Rd_Rs },
    { "LoadColorComp",  Op::LoadColorComp   , OpParams::Rd_Cs_I },
    { "StoreColorComp", Op::StoreColorComp  , OpParams::Cd_I_Rs },
    { "MinInt",         Op::MinInt          , OpParams::None },
    { "MinFloat",       Op::MinFloat        , OpParams::None },
    { "MaxInt",         Op::MaxInt          , OpParams::None },
    { "MaxFloat",       Op::MaxFloat        , OpParams::None },
    { "SetLight",       Op::SetLight        , OpParams::Rd_Cs },
    { "Init",           Op::Init            , OpParams::Id },
    { "RandomInt",      Op::RandomInt       , OpParams::None },
    { "RandomFloat",    Op::RandomFloat     , OpParams::None },
    { "Or",             Op::Or              , OpParams::None },
    { "Xor",            Op::Xor             , OpParams::None },
    { "And",            Op::And             , OpParams::None },
    { "Not",            Op::Not             , OpParams::None },
    { "LOr",            Op::LOr             , OpParams::None },
    { "LAnd",           Op::LAnd            , OpParams::None },
    { "LNot",           Op::LNot            , OpParams::None },
    { "LTInt",          Op::LTInt           , OpParams::None },
    { "LTFloat",        Op::LTFloat         , OpParams::None },
    { "LEInt",          Op::LEInt           , OpParams::None },
    { "LEFloat",        Op::LEFloat         , OpParams::None },
    { "EQInt",          Op::EQInt           , OpParams::None },
    { "EQFloat",        Op::EQFloat         , OpParams::None },
    { "NEInt",          Op::NEInt           , OpParams::None },
    { "NEFloat",        Op::NEFloat         , OpParams::None },
    { "GEInt",          Op::GEInt           , OpParams::None },
    { "GEFloat",        Op::GEFloat         , OpParams::None },
    { "GTInt",          Op::GTInt           , OpParams::None },
    { "GTFloat",        Op::GTFloat         , OpParams::None },
    { "AddInt",         Op::AddInt          , OpParams::None },
    { "AddFloat",       Op::AddFloat        , OpParams::None },
    { "SubInt",         Op::SubInt          , OpParams::None },
    { "SubFloat",       Op::SubFloat        , OpParams::None },
    { "MulInt",         Op::MulInt          , OpParams::None },
    { "MulFloat",       Op::MulFloat        , OpParams::None },
    { "DivInt",         Op::DivInt          , OpParams::None },
    { "DivFloat",       Op::DivFloat        , OpParams::None },
    { "NegInt",         Op::NegInt          , OpParams::None },
    { "NegFloat",       Op::NegFloat        , OpParams::None },
    { "LoadColorParam", Op::LoadColorParam  , OpParams::C_I },
    { "LoadIntParam",   Op::LoadIntParam    , OpParams::R_I },
    { "LoadFloatParam", Op::LoadFloatParam  , OpParams::R_I },
    { "Load",           Op::Load            , OpParams::R_Id },
    { "Store",          Op::Store           , OpParams::Id_R },
    { "LoadBlack",      Op::LoadBlack       , OpParams::C },
    { "LoadZero",       Op::LoadZero        , OpParams::R },
    { "LoadIntConst",   Op::LoadIntConst    , OpParams::R_Const },
    { "Exit",           Op::Exit            , OpParams::R },
    { "Call",           Op::Call            , OpParams::Target },
    { "Return",         Op::Return          , OpParams::None },
    { "ToFloat",        Op::ToFloat         , OpParams::R },
    { "ToInt",          Op::ToInt           , OpParams::R },
    { "SetAllLights",   Op::SetAllLights    , OpParams::C },
    { "foreach",        Op::ForEach         , OpParams::R_Sz },
    { "if",             Op::If              , OpParams::Sz },
    { "else",           Op::Else            , OpParams::Sz },
    { "Log",            Op::Log             , OpParams::R },
    { "LogFloat",       Op::LogFloat        , OpParams::R },
    { "LogColor",       Op::LogColor        , OpParams::C },
};

bool
CompileEngine::program()
{
    try {
        defs();
        constants();
        tables();
        vars();
        functions();
        effects();
    }
    catch(...) {
        return false;
    }
    return true;
}

void
CompileEngine::emit(std::vector<uint8_t>& executable)
{
    executable.push_back('a');
    executable.push_back('r');
    executable.push_back('l');
    executable.push_back('y');
    executable.push_back(_rom32.size());
    executable.push_back(0);
    executable.push_back(0);
    executable.push_back(0);
    
    char* buf = reinterpret_cast<char*>(&(_rom32[0]));
    executable.insert(executable.end(), buf, buf + _rom32.size() * 4);

    for (int i = 0; i < _effects.size(); ++i) {
        executable.push_back(_effects[i]._cmd);
        executable.push_back(_effects[i]._count);
        executable.push_back(uint8_t(_effects[i]._initAddr));
        executable.push_back(uint8_t(_effects[i]._initAddr >> 8));
        executable.push_back(uint8_t(_effects[i]._loopAddr));
        executable.push_back(uint8_t(_effects[i]._loopAddr >> 8));
    }
    executable.push_back(0);
    
    buf = reinterpret_cast<char*>(&(_rom8[0]));
    executable.insert(executable.end(), buf, buf + _rom8.size());
}

bool
CompileEngine::opcode(Op op, std::string& str, OpParams& par)
{
    OpData data;
    if (!opDataFromOp(op, data)) {
        return false;
    }
    str = data._str;
    par = data._par;
    return true;
}

void
CompileEngine::defs()
{
    while(1) {
        ignoreNewLines();
        if (!def()) {
            return;
        }
        expect(Token::NewLine);
    }
}

void
CompileEngine::constants()
{
    while(1) {
        ignoreNewLines();
        if (!constant()) {
            return;
        }
        expect(Token::NewLine);
    }
}

void
CompileEngine::tables()
{
    while(1) {
        ignoreNewLines();
        if (!table()) {
            return;
        }
        expect(Token::NewLine);
    }
}

void
CompileEngine::functions()
{
    while(1) {
        ignoreNewLines();
        if (!function()) {
            return;
        }
        expect(Token::NewLine);
    }
}

void
CompileEngine::effects()
{
    while(1) {
        ignoreNewLines();
        if (!effect()) {
            return;
        }
        expect(Token::NewLine);
    }
}

bool
CompileEngine::def()
{
    if (!match(Reserved::Def)) {
        return false;
    }
    
    std::string id;
    int32_t val;
    
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    expect(integerValue(val), Compiler::Error::ExpectedValue);
    
    // A def is always a positive integer less than 256
    expect(val >= 0 && val < 256, Compiler::Error::DefOutOfRange);
    
    _defs.emplace_back(id, val);
    return true;
}

bool
CompileEngine::constant()
{
    if (!match(Reserved::Const)) {
        return false;
    }
    
    Type t;
    std::string id;
    int32_t val;
    
    expect(type(t), Compiler::Error::ExpectedType);
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    expect(value(val, t), Compiler::Error::ExpectedValue);
    
    // There is only enough room for 128 constant values
    expect(_rom32.size() < 128, Compiler::Error::TooManyConstants);

    // Save constant
    _symbols.emplace_back(id, _rom32.size(), true);
    _rom32.push_back(val);
    
    return true;
}

bool
CompileEngine::table()
{
    if (!match(Reserved::Table)) {
        return false;
    }
    
    Type t;
    std::string id;
    expect(type(t), Compiler::Error::ExpectedType);
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    expect(Token::NewLine);
    
    // Set the start address of the table. tableEntries() will fill them in
    _symbols.emplace_back(id, _rom32.size(), true);
    
    ignoreNewLines();
    tableEntries(t);
    expect(Token::Identifier, "end");
    return true;
}

bool
CompileEngine::function()
{
    if (!match(Reserved::Function)) {
        return false;
    }
    
    std::string id;

    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    
    // Remember the function
    _functions.emplace_back(id, uint16_t(_rom8.size()));
    
    ignoreNewLines();
    
    statements();
    
    expect(Token::Identifier, "end");
    
    // Insert a return at the end of the function to make sure it returns
    _rom8.push_back(uint8_t(Op::Return));
    return true;
}

bool
CompileEngine::effect()
{
    if (!match(Reserved::Effect)) {
        return false;
    }

    std::string id;

    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    
    // Effect Identifier must be a single char from 'a' to 'p'
    if (id.size() != 1 || id[0] < 'a' || id[0] > 'p') {
        _error = Compiler::Error::ExpectedCommandId;
        throw true;
    }

    expect(Token::Integer);
    
    int32_t paramCount = _scanner.getTokenValue().integer;
    
    // paramCount must be 0 - 15
    if (paramCount < 0 || paramCount > 15) {
        _error = Compiler::Error::InvalidParamCount;
        throw true;
    }
    
    _effects.emplace_back(id[0], paramCount, handleCallTarget(), handleCallTarget());
    return true;
}

bool
CompileEngine::type(Type& t)
{
    if (match(Reserved::Float)) {
        t = Type::Float;
        return true;
    }
    if (match(Reserved::Int)) {
        t = Type::Int;
        return true;
    }
    return false;
}

void
CompileEngine::tableEntries(Type t)
{
    while (1) {
        ignoreNewLines();
        if (!values(t)) {
            break;
        }
        expect(Token::NewLine);
    }
}

bool
CompileEngine::values(Type t)
{
    bool haveValues = false;
    while(1) {
        int32_t val;
        if (!value(val, t)) {
            break;
        }
        haveValues = true;
        
        _rom32.push_back(val);
    }
    return haveValues;
}

bool
CompileEngine::value(int32_t& i, Type t)
{
    bool neg = false;
    if (match(Token::Minus)) {
        neg = true;
    }
    
    float f;
    if (floatValue(f)) {
        if (neg) {
            f = -f;
        }

        // If we're expecting an Integer, convert it
        if (t == Type::Int) {
            i = roundf(f);
        } else {
            i = *(reinterpret_cast<int32_t*>(&f));
        }
        return true;
    }
    
    if (integerValue(i)) {
        if (neg) {
            i = -i;
        }
        
        // If we're expecting a float, convert it
        if (t == Type::Float) {
            f = float(i);
            i = *(reinterpret_cast<int32_t*>(&f));
        }
        return true;
    }
    return false;
}

void
CompileEngine::vars()
{
    while(1) {
        ignoreNewLines();
        if (!var()) {
            return;
        }
        expect(Token::NewLine);
    }
}

bool
CompileEngine::var()
{
    Type t;
    std::string id;
    
    if (!type(t)) {
        return false;
    }
    
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    
    int32_t size;
    expect(integerValue(size), Compiler::Error::ExpectedInt);

    _symbols.emplace_back(id, _nextMem, false);
    _nextMem += size;

    // There is only enough room for 128 var values
    expect(_nextMem <= 128, Compiler::Error::TooManyVars);

    return true;
}

bool
CompileEngine::init()
{
    ignoreNewLines();

    if (!match(Reserved::Init)) {
        return false;
    }
    expect(Token::NewLine);
    
    _effects.back()._initAddr = uint16_t(_rom8.size());
    
    statements();

    ignoreNewLines();
    expect(match(Reserved::End), Compiler::Error::ExpectedEnd);
    expect(Token::NewLine);

    _rom8.push_back(uint8_t(Op::End));
    return true;
}

bool
CompileEngine::loop()
{
    ignoreNewLines();

    if (!match(Reserved::Loop)) {
        return false;
    }
    expect(Token::NewLine);

    _effects.back()._loopAddr = uint16_t(_rom8.size());

    statements();

    ignoreNewLines();
    expect(match(Reserved::End), Compiler::Error::ExpectedEnd);
    expect(Token::NewLine);

    _rom8.push_back(uint8_t(Op::End));
    return true;
}

void
CompileEngine::statements()
{
    while(1) {
        ignoreNewLines();
        if (!statement()) {
            return;
        }
        expect(Token::NewLine);
    }
}

bool
CompileEngine::statement()
{
    if (forStatement()) {
        return true;
    }
    if (ifStatement()) {
        return true;
    }
    if (opStatement()) {
        return true;
    }
    return false;
}

uint8_t
CompileEngine::handleR()
{
    uint8_t i = 0;
    if (match(Reserved::R0)) {
        i = 0x00;
    } else if (match(Reserved::R1)) {
        i = 0x01;
    } else if (match(Reserved::R2)) {
        i = 0x02;
    } else if (match(Reserved::R3)) {
        i = 0x03;
    } else {
        expect(false, Compiler::Error::ExpectedRegister);
    }
    return i;
}

uint8_t
CompileEngine::handleR(Op op)
{
    return uint8_t(op) | handleR();
}

uint8_t
CompileEngine::handleC()
{
    uint8_t i = 0;
    if (match(Reserved::C0)) {
        i = 0x00;
    } else if (match(Reserved::C1)) {
        i = 0x01;
    } else if (match(Reserved::C2)) {
        i = 0x02;
    } else if (match(Reserved::C3)) {
        i = 0x03;
    } else {
        expect(false, Compiler::Error::ExpectedRegister);
    }
    return i;
}

uint8_t
CompileEngine::handleC(Op op)
{
    return uint8_t(op) | handleC();
}

uint8_t
CompileEngine::handleI()
{
    uint8_t i = handleConst();
    expect(i <= 15, Compiler::Error::ParamOutOfRange);
    return uint8_t(i);
}

uint8_t
CompileEngine::handleConst()
{
    int32_t i;
    std::string id;
    
    if (identifier(id)) {
        // See if this is a def
        auto it = find_if(_defs.begin(), _defs.end(),
                    [id](const Def& d) { return d._name == id; });
        expect(it != _defs.end(), Compiler::Error::ExpectedDef);
        
        i = it->_value;
    } else {
        expect(integerValue(i), Compiler::Error::ExpectedInt);
    }

    expect(i >= 0 && i < 256, Compiler::Error::ParamOutOfRange);
    return uint8_t(i);
}

uint8_t
CompileEngine::handleId()
{
    std::string id;
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    
    auto it = find_if(_symbols.begin(), _symbols.end(),
                    [id](const Symbol& sym) { return sym._name == id; });
    expect(it != _symbols.end(), Compiler::Error::UndefinedIdentifier);

    // var memory (RAM) starts at 0x80.
    return it->_rom ? it->_addr : (it->_addr + 0x80);
}

uint16_t
CompileEngine::handleCallTarget()
{
    std::string targ;
    expect(identifier(targ), Compiler::Error::ExpectedIdentifier);
    
    auto it = find_if(_functions.begin(), _functions.end(),
                    [targ](const Function& fun) { return fun._name == targ; });
    expect(it != _functions.end(), Compiler::Error::UndefinedIdentifier);

    return it->_addr;
}

void
CompileEngine::handleOpParams(uint8_t a)
{
    _rom8.push_back(a);
    expectWithoutRetire(Token::NewLine);
}

void
CompileEngine::handleOpParams(uint8_t a, uint8_t b)
{
    _rom8.push_back(a);
    _rom8.push_back(b);
    expectWithoutRetire(Token::NewLine);
}

void
CompileEngine::handleOpParamsReverse(uint8_t a, uint8_t b)
{
    _rom8.push_back(b);
    _rom8.push_back(a);
    expectWithoutRetire(Token::NewLine);
}

void
CompileEngine::handleOpParamsRdRs(Op op, uint8_t rd, uint8_t rs)
{
    _rom8.push_back(uint8_t(op));
    _rom8.push_back((rd << 6) | (rs << 4));
    expectWithoutRetire(Token::NewLine);
}

void
CompileEngine::handleOpParamsRdRsI(Op op, uint8_t rd, uint8_t rs, uint8_t i)
{
    _rom8.push_back(uint8_t(op));
    _rom8.push_back((rd << 6) | (rs << 4) | (i & 0x0f));
    expectWithoutRetire(Token::NewLine);
}

void
CompileEngine::handleOpParamsRdIRs(Op op, uint8_t rd, uint8_t i, uint8_t rs)
{
    _rom8.push_back(uint8_t(op));
    _rom8.push_back((rd << 6) | (rs << 4) | (i & 0x0f));
    expectWithoutRetire(Token::NewLine);
}

void
CompileEngine::handleOpParamsRdRs(Op op, uint8_t id, uint8_t rd, uint8_t i, uint8_t rs)
{
    _rom8.push_back(uint8_t(op));
    _rom8.push_back(id);
    _rom8.push_back((rd << 6) | (rs << 4) | (i & 0x0f));
    expectWithoutRetire(Token::NewLine);
}

void
CompileEngine::handleOpParamsRdRsSplit(Op op, uint8_t rd, uint8_t id, uint8_t rs, uint8_t i)
{
    _rom8.push_back(uint8_t(op));
    _rom8.push_back(id);
    _rom8.push_back((rd << 6) | (rs << 4) | (i & 0x0f));
    expectWithoutRetire(Token::NewLine);
}

bool
CompileEngine::opStatement()
{
    Op op;
    OpParams par;
    if (!opcode(_scanner.getToken(), _scanner.getTokenString(), op, par)) {
        return false;
    }
    
    // Don't handle Else here
    if (op == Op::Else) {
        return false;
    }
    
    _scanner.retireToken();
    
    // Get the params in the sequence specified in OpParams
    switch(par) {
        case OpParams::None: handleOpParams(uint8_t(op)); break;
        case OpParams::R: handleOpParams(handleR(op)); break;
        case OpParams::C: handleOpParams(handleC(op)); break;
        case OpParams::R_I: handleOpParams(handleR(op), handleI()); break;
        case OpParams::C_I: handleOpParams(handleC(op), handleI()); break;
        case OpParams::R_Id: handleOpParams(handleR(op), handleId()); break;
        case OpParams::C_Id: handleOpParams(handleC(op), handleId()); break;
        
        case OpParams::Id_R: handleOpParamsReverse(handleId(), handleR(op)); break;
        case OpParams::Id_C: handleOpParamsReverse(handleId(), handleC(op)); break;

        case OpParams::Rd_Rs: handleOpParamsRdRs(op, handleR(), handleR()); break;
        case OpParams::Cd_Rs: handleOpParamsRdRs(op, handleC(), handleR()); break;
        case OpParams::Rd_Cs: handleOpParamsRdRs(op, handleR(), handleC()); break;
        case OpParams::Cd_Cs: handleOpParamsRdRs(op, handleC(), handleC()); break;

        case OpParams::Cd_Id_Rs_I:
            handleOpParamsRdRsSplit(op, handleC(), handleId(), handleR(), handleI());
            break;
        case OpParams::Rd_Id_Rs_I:
            handleOpParamsRdRsSplit(op, handleR(), handleId(), handleR(), handleI());
            break;
        case OpParams::Id_Rd_I_Cs:
            handleOpParamsRdRs(op, handleId(), handleR(), handleI(), handleC());
            break;
        case OpParams::Id_Rd_I_Rs:
            handleOpParamsRdRs(op, handleId(), handleR(), handleI(), handleR());
            break;
        case OpParams::Rd_Rs_I:
            handleOpParamsRdRsI(op, handleR(), handleR(), handleI());
            break;
        case OpParams::Rd_Cs_I:
            handleOpParamsRdRsI(op, handleR(), handleC(), handleI());
            break;
        case OpParams::Rd_I_Rs:
            handleOpParamsRdIRs(op, handleR(), handleI(), handleR());
            break;
        case OpParams::Cd_I_Rs:
            handleOpParamsRdIRs(op, handleC(), handleI(), handleR());
            break;
        case OpParams::Id:
            _rom8.push_back(uint8_t(op));
            _rom8.push_back(handleId());
            expectWithoutRetire(Token::NewLine);
            break;
        case OpParams::Target: {
            uint16_t targ = handleCallTarget();
            _rom8.push_back(uint8_t(op) | uint8_t(targ & 0x03));
            _rom8.push_back(uint8_t(targ >> 2));
            break;
        }
        case OpParams::R_Const:
            handleOpParams(handleR(op), handleConst()); break;
            break;
        case OpParams::R_Sz:
        case OpParams::Sz:
            // Should never get here
            break;
    }
    return true;
}

bool
CompileEngine::forStatement()
{
    if (!match(Reserved::ForEach)) {
        return false;
    }
    
    Reserved reg;
    expect(reserved(reg), Compiler::Error::ExpectedRegister);
    _scanner.retireToken();
    expect(Token::NewLine);
    
    // Generate ForEach and add in reg
    uint8_t i = 0;
    if (reg == Reserved::R0) {
    } else if (reg == Reserved::R1) {
        i = 0x01;
    } else if (reg == Reserved::R2) {
        i = 0x02;
    } else if (reg == Reserved::R3) {
        i = 0x03;
    } else {
        expect(false, Compiler::Error::ExpectedRegister);
    }
    
    _rom8.push_back(uint8_t(Op::ForEach) | i);
    
    // Output a placeholder for sz and rember where it is
    auto szIndex = _rom8.size();
    _rom8.push_back(0);
    
    statements();
    expect(match(Reserved::End), Compiler::Error::ExpectedEnd);
    
    // Update sz
    // rom is pointing at inst past 'end', we want to point at end
    auto offset = _rom8.size() - szIndex - 1;
    expect(offset < 256, Compiler::Error::ForEachTooBig);
    
    _rom8[szIndex] = uint8_t(offset);
    
    // Push a dummy end, mostly for the decompiler
    _rom8.push_back(uint8_t(Op::EndForEach));

    return true;
}

bool
CompileEngine::ifStatement()
{
    if (!match(Reserved::If)) {
        return false;
    }
    expect(Token::NewLine);

    _rom8.push_back(uint8_t(Op::If));
    
    // Output a placeholder for sz and rember where it is
    auto szIndex = _rom8.size();
    _rom8.push_back(0);
    
    statements();
    
    // Update sz
    auto offset = _rom8.size() - szIndex - 1;
    expect(offset < 256, Compiler::Error::ForEachTooBig);
    
    _rom8[szIndex] = uint8_t(offset);
    
    if (match(Reserved::Else)) {
        expect(Token::NewLine);

        _rom8.push_back(uint8_t(Op::Else));
    
        // Output a placeholder for sz and rember where it is
        auto szIndex = _rom8.size();
        _rom8.push_back(0);
        statements();

        // Update sz
        // rom is pointing at inst past 'end', we want to point at end
        auto offset = _rom8.size() - szIndex - 1;
        expect(offset < 256, Compiler::Error::ForEachTooBig);
    
        _rom8[szIndex] = uint8_t(offset);
    }
    
    expect(match(Reserved::End), Compiler::Error::ExpectedEnd);

    // Finally output and EndIf. This lets us distinguish
    // Between an if and an if-else. If we skip an If we
    // will either see an Else of an EndIf instruction.
    // If we see anything else it's an error. If we see
    // an Else, it means this is the else clause of an
    // if statement we've skipped, so we execute its
    // statements. If we see an EndIf it means this If
    // doesn't have an Else.
    _rom8.push_back(uint8_t(Op::EndIf));

    return true;
}

void
CompileEngine::expect(Token token, const char* str)
{
    bool err = false;
    if (_scanner.getToken() != token) {
        _error = Compiler::Error::ExpectedToken;
        err = true;
    }
    
    if (str && _scanner.getTokenString() != str) {
        _error = Compiler::Error::ExpectedToken;
        err = true;
    }
    
    if (err) {
        _expectedToken = token;
        _expectedString = str ? : "";
        throw true;
    }

    _scanner.retireToken();
}

void
CompileEngine::expect(bool passed, Compiler::Error error)
{
    if (!passed) {
        _error = error;
        throw true;
    }
}

void
CompileEngine::expectWithoutRetire(Token token)
{
    if (_scanner.getToken() != token) {
        _expectedToken = token;
        _expectedString = "";
        _error = Compiler::Error::ExpectedToken;
        throw true;
    }
}

bool
CompileEngine::match(Reserved r)
{
    Reserved rr;
    if (!isReserved(_scanner.getToken(), _scanner.getTokenString(), rr)) {
        return false;
    }
    if (r != rr) {
        return false;
    }
    _scanner.retireToken();
    return true;
}

bool
CompileEngine::match(Token t)
{
    if (_scanner.getToken() != t) {
        return false;
    }
    _scanner.retireToken();
    return true;
}

void
CompileEngine::ignoreNewLines()
{
    while (1) {
        if (_scanner.getToken() != Token::NewLine) {
            break;
        }
        _scanner.retireToken();
    }
}

bool
CompileEngine::identifier(std::string& id)
{
    if (_scanner.getToken() != Token::Identifier) {
        return false;
    }
    
    if (opcode() || reserved()) {
        return false;
    }
    
    id = _scanner.getTokenString();
    _scanner.retireToken();
    return true;
}

bool
CompileEngine::integerValue(int32_t& i)
{
    if (_scanner.getToken() != Token::Integer) {
        return false;
    }
    
    i = _scanner.getTokenValue().integer;
    _scanner.retireToken();
    return true;
}

bool
CompileEngine::floatValue(float& f)
{
    if (_scanner.getToken() != Token::Float) {
        return false;
    }
    
    f = _scanner.getTokenValue().number;
    _scanner.retireToken();
    return true;
}

bool
CompileEngine::opcode()
{
    return opcode(_scanner.getToken(), _scanner.getTokenString());
}

bool
CompileEngine::reserved()
{
    Reserved r;
    return isReserved(_scanner.getToken(), _scanner.getTokenString(), r);
}

bool
CompileEngine::reserved(Reserved &r)
{
    return isReserved(_scanner.getToken(), _scanner.getTokenString(), r);
}

bool
CompileEngine::opcode(Token token, const std::string str)
{
    Op op;
    OpParams par;
    return opcode(token, str, op, par);
}

bool
CompileEngine::opcode(Token token, const std::string str, Op& op, OpParams& par)
{
    if (token != Token::Identifier) {
        return false;
    }
    
    OpData data;
    if (!opDataFromString(str, data)) {
        return false;
    }
    op = data._op;
    par = data._par;
    return true;
}

bool
CompileEngine::isReserved(Token token, const std::string str, Reserved& r)
{
    static std::map<std::string, Reserved> reserved = {
        { "const",      Reserved::Const },
        { "table",      Reserved::Table },
        { "effect",     Reserved::Effect },
        { "end",        Reserved::End },
        { "init",       Reserved::Init },
        { "loop",       Reserved::Loop },
        { "foreach",    Reserved::ForEach },
        { "if",         Reserved::If },
        { "else",       Reserved::Else },
        { "float",      Reserved::Float },
        { "int",        Reserved::Int },
        { "r0",         Reserved::R0 },
        { "r1",         Reserved::R1 },
        { "r2",         Reserved::R2 },
        { "r3",         Reserved::R3 },
        { "c0",         Reserved::C0 },
        { "c1",         Reserved::C1 },
        { "c2",         Reserved::C2 },
        { "c3",         Reserved::C3 },
        { "function",   Reserved::Function },
        { "def",        Reserved::Def },
    };

    if (token != Token::Identifier) {
        return false;
    }
    
    auto it = reserved.find(str);
    if (it != reserved.end()) {
        r = it->second;
        return true;
    }
    return false;
}

bool
CompileEngine::opDataFromString(const std::string str, OpData& data)
{
    auto it = find_if(_opcodes.begin(), _opcodes.end(),
                    [str](const OpData& opData) { return opData._str == str; });
    if (it != _opcodes.end()) {
        data = *it;
        return true;
    }
    return false;
}

bool
CompileEngine::opDataFromOp(const Op op, OpData& data)
{
    auto it = find_if(_opcodes.begin(), _opcodes.end(),
                    [op](const OpData& opData) { return opData._op == op; });
    if (it != _opcodes.end()) {
        data = *it;
        return true;
    }
    return false;
}
