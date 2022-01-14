//
//  CompileEngine.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "CompileEngine.h"

#include <map>

using namespace arly;

std::vector<OpData> CompileEngine::_opcodes = {
    { "LoadColorX",     Op::LoadColor        , OpParams::Cd_Id_Rs },
    { "LoadX",          Op::LoadX            , OpParams::Rd_Id_Rs_I },
    { "StoreColorX",    Op::StoreColorX      , OpParams::Id_Rd_Cs },
    { "StoreX",         Op::StoreX           , OpParams::Id_Rd_I_Rs },
    { "MoveColor",      Op::MoveColor        , OpParams::Cd_Cs },
    { "Move",           Op::Move             , OpParams::Rd_Rs },
    { "LoadVal",        Op::LoadVal          , OpParams::Rd_Cs },
    { "StoreVal",       Op::StoreVal         , OpParams::Cd_Rs },
    { "MinInt",         Op::MinInt           , OpParams::None },
    { "MinFloat",       Op::MinFloat         , OpParams::None },
    { "MaxInt",         Op::MaxInt           , OpParams::None },
    { "MaxFloat",       Op::MaxFloat         , OpParams::None },
    { "SetLight",       Op::SetLight         , OpParams::Rd_Cs },
    { "Init",           Op::Init             , OpParams::Id },
    { "Random",         Op::Random           , OpParams::None },
    { "Bor",            Op::Bor              , OpParams::None },
    { "Bxor",           Op::Bxor             , OpParams::None },
    { "Band",           Op::Band             , OpParams::None },
    { "Bnot",           Op::Bnot             , OpParams::None },
    { "Or",             Op::Or               , OpParams::None },
    { "And",            Op::And              , OpParams::None },
    { "Not",            Op::Not              , OpParams::None },
    { "LTInt",          Op::LTInt            , OpParams::None },
    { "LTFloat",        Op::LTFloat          , OpParams::None },
    { "LEInt",          Op::LEInt            , OpParams::None },
    { "LEFloat",        Op::LEFloat          , OpParams::None },
    { "EQInt",          Op::EQInt            , OpParams::None },
    { "EQFloat",        Op::EQFloat          , OpParams::None },
    { "NEInt",          Op::NEInt            , OpParams::None },
    { "NEFloat",        Op::NEFloat          , OpParams::None },
    { "GEInt",          Op::GEInt            , OpParams::None },
    { "GEFloat",        Op::GEFloat          , OpParams::None },
    { "GTInt",          Op::GTInt            , OpParams::None },
    { "GTFloat",        Op::GTFloat          , OpParams::None },
    { "AddInt",         Op::AddInt           , OpParams::None },
    { "AddFloat",       Op::AddFloat         , OpParams::None },
    { "SubInt",         Op::SubInt           , OpParams::None },
    { "SubFloat",       Op::SubFloat         , OpParams::None },
    { "MulInt",         Op::MulInt           , OpParams::None },
    { "MulFloat",       Op::MulFloat         , OpParams::None },
    { "DivInt",         Op::DivInt           , OpParams::None },
    { "DivFloat",       Op::DivFloat         , OpParams::None },
    { "NegInt",         Op::NegInt           , OpParams::None },
    { "NegFloat",       Op::NegFloat         , OpParams::None },
    { "LoadColorParam", Op::LoadColorParam   , OpParams::C_I },
    { "LoadIntParam",   Op::LoadIntParam     , OpParams::R_I },
    { "LoadFloatParam", Op::LoadFloatParam   , OpParams::R_I },
    { "LoadColor",      Op::LoadColor        , OpParams::C_Id },
    { "Load",           Op::Load             , OpParams::R_Id },
    { "StoreColor",     Op::StoreColor       , OpParams::Id_C },
    { "Store",          Op::Store            , OpParams::Id_R },
    { "LoadBlack",      Op::LoadBlack        , OpParams::C },
    { "LoadZero",       Op::LoadZero         , OpParams::R },
    { "LoadIntOne",     Op::LoadIntOne       , OpParams::R },
    { "LoadFloatOne",   Op::LoadFloatOne     , OpParams::R },
    { "LoadByteMax",    Op::LoadByteMax      , OpParams::R },
    { "Return",         Op::Return           , OpParams::R },
    { "ToFloat",        Op::ToFloat          , OpParams::R },
    { "ToInt",          Op::ToInt            , OpParams::R },
    { "SetAllLights",   Op::SetAllLights     , OpParams::C },
    { "foreach",        Op::ForEach          , OpParams::R },
};

bool
CompileEngine::program()
{
    try {
        constants();
        tables();
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
CompileEngine::constants()
{
    while(1) {
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
        if (!table()) {
            return;
        }
        expect(Token::NewLine);
    }
}

void
CompileEngine::effects()
{
    while(1) {
        if (!effect()) {
            return;
        }
        expect(Token::NewLine);
    }
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
    expect(value(val), Compiler::Error::ExpectedValue);
    
    // Save constant
    _symbols.emplace_back(id, _rom32.size());
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
    _symbols.emplace_back(id, _rom32.size());
    
    tableEntries();
    expect(Token::Identifier, "end");
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
    
    _effects.emplace_back(id[0], paramCount);

    expect(Token::NewLine);
    
    defs();
    init();
    loop();
    expect(match(Reserved::End), Compiler::Error::ExpectedEnd);
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
CompileEngine::tableEntries()
{
    while (1) {
        if (!values()) {
            break;
        }
        expect(Token::NewLine);
    }
}

bool
CompileEngine::values()
{
    bool haveValues = false;
    while(1) {
        int32_t val;
        if (!value(val)) {
            break;
        }
        haveValues = true;
        
        _rom32.push_back(val);
    }
    return haveValues;
}

bool
CompileEngine::value(int32_t& i)
{
    if (match(Token::Float)) {
        float f = _scanner.getTokenValue().number;
        i = *(reinterpret_cast<int32_t*>(&f));
        return true;
    }
    if (match(Token::Integer)) {
        i = _scanner.getTokenValue().integer;
        return true;
    }
    return false;
}

bool
CompileEngine::integer(int32_t& i)
{
    if (match(Token::Integer)) {
        i = _scanner.getTokenValue().integer;
        return true;
    }
    return false;
}

void
CompileEngine::defs()
{
    while(1) {
        if (!def()) {
            return;
        }
        expect(Token::NewLine);
    }
}

bool
CompileEngine::def()
{
    Type t;
    std::string id;
    
    if (!type(t)) {
        return false;
    }
    
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    
    int32_t size;
    expect(integer(size), Compiler::Error::ExpectedInt);

    _symbols.emplace_back(id, _nextMem);
    _nextMem += size;

    return true;
}

bool
CompileEngine::init()
{
    if (!match(Reserved::Init)) {
        return false;
    }
    expect(Token::NewLine);
    
    _effects.back()._initAddr = uint16_t(_rom8.size());
    
    statements();
    expect(match(Reserved::End), Compiler::Error::ExpectedEnd);
    expect(Token::NewLine);

    _rom8.push_back(uint8_t(Op::End));
    return true;
}

bool
CompileEngine::loop()
{
    if (!match(Reserved::Loop)) {
        return false;
    }
    expect(Token::NewLine);

    _effects.back()._loopAddr = uint16_t(_rom8.size());

    statements();
    expect(match(Reserved::End), Compiler::Error::ExpectedEnd);
    expect(Token::NewLine);

    _rom8.push_back(uint8_t(Op::End));
    return true;
}

void
CompileEngine::statements()
{
    while(1) {
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
    int32_t i;
    expect(integer(i), Compiler::Error::ExpectedInt);
    expect(i >= 0 && i <= 15, Compiler::Error::ParamOutOfRange);
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
CompileEngine::handleOpParamsRdRs(Op op, uint8_t a, uint8_t rd, uint8_t rs)
{
    _rom8.push_back(uint8_t(op));
    _rom8.push_back(a);
    _rom8.push_back((rd << 6) | (rs << 4));
    expectWithoutRetire(Token::NewLine);
}

void
CompileEngine::handleOpParamsRdRsSplit(Op op, uint8_t rd, uint8_t a, uint8_t rs)
{
    _rom8.push_back(uint8_t(op));
    _rom8.push_back(a);
    _rom8.push_back((rd << 6) | (rs << 4));
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
    
    _scanner.retireToken();
    
    // Get the params in the sequence specified in OpParams
    uint8_t i;
    
    switch(par) {
        case OpParams::None: expectWithoutRetire(Token::NewLine); break;
        case OpParams::R: handleOpParams(handleR(op)); break;
        case OpParams::C: handleOpParams(handleC(op)); break;
        case OpParams::R_I: handleOpParams(handleR(op), handleI()); break;
        case OpParams::C_I: handleOpParams(handleC(op), handleI()); break;
        case OpParams::R_Id: handleOpParams(handleR(op), handleId()); break;
        case OpParams::C_Id: handleOpParams(handleC(op), handleId()); break;
        
        case OpParams::Id_R: handleOpParamsReverse(handleId(), handleR(op)); break;
        case OpParams::Id_C: handleOpParamsReverse(handleId(), handleC(op)); break;

        case OpParams::Rd_Id_Rs: handleOpParamsRdRsSplit(op, handleR(), handleId(), handleR()); break;
        case OpParams::Cd_Id_Rs: handleOpParamsRdRsSplit(op, handleC(), handleId(), handleR()); break;

        case OpParams::Id_Rd_Rs: handleOpParamsRdRs(op, handleId(), handleR(), handleR()); break;
        case OpParams::Id_Rd_Cs: handleOpParamsRdRs(op, handleId(), handleR(), handleC()); break;
        case OpParams::Rd_Rs: handleOpParamsRdRs(op, handleR(), handleR()); break;
        case OpParams::Cd_Rs: handleOpParamsRdRs(op, handleC(), handleR()); break;
        case OpParams::Rd_Cs: handleOpParamsRdRs(op, handleR(), handleC()); break;
        case OpParams::Cd_Cs: handleOpParamsRdRs(op, handleC(), handleC()); break;

        case OpParams::Rd_Id_Rs_I:
            _rom8.push_back(uint8_t(op));
            i = handleR();
            _rom8.push_back(handleId());
            _rom8.push_back((i << 6) | (handleR() << 4) | handleI());
            expectWithoutRetire(Token::NewLine);
            break;
        case OpParams::Id_Rd_I_Rs:
            _rom8.push_back(uint8_t(op));
            _rom8.push_back(handleId());
            i = handleR();
            _rom8.push_back((i << 6) | handleI() | (handleR() << 4));
            expectWithoutRetire(Token::NewLine);
            break;
        case OpParams::Id:
            _rom8.push_back(uint8_t(op));
            _rom8.push_back(handleId());
            expectWithoutRetire(Token::NewLine);
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
    auto offset = _rom8.size() - szIndex;
    expect(offset < 256, Compiler::Error::ForEachTooBig);
    
    _rom8[szIndex] = uint8_t(offset);
    
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
    
    if (match(Reserved::Else)) {
        expect(Token::NewLine);

        _rom8.push_back(uint8_t(Op::Else));
    
        // Output a placeholder for sz and rember where it is
        auto szIndex = _rom8.size();
        _rom8.push_back(0);
        statements();

        // Update sz
        auto offset = _rom8.size() - szIndex;
        expect(offset < 256, Compiler::Error::ForEachTooBig);
    
        _rom8[szIndex] = uint8_t(offset);
    }
    
    expect(match(Reserved::End), Compiler::Error::ExpectedEnd);

    // Update sz
    auto offset = _rom8.size() - szIndex;
    expect(offset < 256, Compiler::Error::ForEachTooBig);
    
    _rom8[szIndex] = uint8_t(offset);
    
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
        throw true;
    }
}

bool
CompileEngine::match(Token token, const char* str)
{
    if (_scanner.getToken() != token) {
        return false;
    }
    
    if (str && _scanner.getTokenString() != str) {
        return false;
    }
    
    _scanner.retireToken();
    return true;
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
