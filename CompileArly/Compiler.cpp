//
//  Compiler.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Compiler.h"

#include <map>
#include <vector>

using namespace arly;

class CompileEngine
{
public:
    CompileEngine(std::istream* stream) : _scanner(stream) { }
    
    bool program()
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
    
    void emit(std::vector<uint8_t> executable)
    {
        executable.push_back('a');
        executable.push_back('r');
        executable.push_back('l');
        executable.push_back('y');
        executable.push_back(_rom32.size() + _rom8.size() / 4);
        executable.push_back(0);
        executable.push_back(0);
        executable.push_back(0);
        
        char* buf = reinterpret_cast<char*>(&(_rom32[0]));
        executable.insert(executable.end(), buf, buf + _rom32.size() * 4);

        for (int i = 0; i < _effects.size(); ++i) {
            executable.push_back(_effects[i]._cmd);
            executable.push_back(_effects[i]._count);
            executable.push_back(_effects[i]._initAddr);
            executable.push_back(_effects[i]._loopAddr);
        }
        
        buf = reinterpret_cast<char*>(&(_rom8[0]));
        executable.insert(executable.end(), buf, buf + _rom8.size());
    }

    bool opcode(Op op, std::string& str, OpParams& par)
    {
        OpData data;
        if (!opDataFromOp(op, data)) {
            return false;
        }
        str = data._str;
        par = data._par;
        return true;
    }
    
    Compiler::Error error() const { return _error; }
    Token expectedToken() const { return _expectedToken; }
    const std::string& expectedString() const { return _expectedString; }
    uint32_t lineno() const { return _scanner.lineno(); }
    
private:
    enum class Type { Float, Int };
    
    void constants()
    {
        while(1) {
            if (!constant()) {
                return;
            }
            expect(Token::NewLine);
        }
    }
    
    void tables()
    {
        while(1) {
            if (!table()) {
                return;
            }
            expect(Token::NewLine);
        }
    }
    
    void effects()
    {
        while(1) {
            if (!effect()) {
                return;
            }
            expect(Token::NewLine);
        }
    }
    
    bool constant()
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
        _symbols.emplace(id, _rom32.size());
        _rom32.push_back(val);
        
        return true;
    }
    
    bool table()
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
        _symbols.emplace(id, _rom32.size());
        
        tableEntries();
        expect(Token::Identifier, "end");
        return true;
    }
    
    bool effect()
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
    
    bool type(Type& t)
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
    
    void tableEntries()
    {
        while (1) {
            if (!values()) {
                break;
            }
            expect(Token::NewLine);
        }
    }
    
    bool values()
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

    // Value is returned as an int32_t, but it might be a float
    bool value(int32_t& i)
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
    
    bool integer(int32_t& i)
    {
        if (match(Token::Integer)) {
            i = _scanner.getTokenValue().integer;
            return true;
        }
        return false;
    }
    
    void defs()
    {
        while(1) {
            if (!def()) {
                return;
            }
            expect(Token::NewLine);
        }
    }
    
    bool def()
    {
        Type t;
        std::string id;
        
        if (!type(t)) {
            return false;
        }
        
        expect(identifier(id), Compiler::Error::ExpectedIdentifier);
        
        int32_t size;
        expect(integer(size), Compiler::Error::ExpectedInt);

        _symbols.emplace(id, _nextMem);
        _nextMem += size;

        return true;
    }

    bool init()
    {
        if (!match(Reserved::Init)) {
            return false;
        }
        expect(Token::NewLine);
        
        _effects.back()._initAddr = _rom8.size() / 4;
        
        statements();
        expect(match(Reserved::End), Compiler::Error::ExpectedEnd);
        expect(Token::NewLine);
        return true;
    }

    bool loop()
    {
        if (!match(Reserved::Loop)) {
            return false;
        }
        expect(Token::NewLine);

        _effects.back()._loopAddr = _rom8.size() / 4;

        statements();
        expect(match(Reserved::End), Compiler::Error::ExpectedEnd);
        expect(Token::NewLine);
        return true;
    }
    
    void statements()
    {
        while(1) {
            if (!statement()) {
                return;
            }
            expect(Token::NewLine);
        }
    }
    
    bool statement()
    {
        if (opStatement()) {
            return true;
        }
        if (forStatement()) {
            return true;
        }
        if (ifStatement()) {
            return true;
        }
        return false;
    }
    
    uint8_t handleR()
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

    uint8_t handleR(Op op)
    {
        return uint8_t(op) | handleR();
    }
    
    uint8_t handleC()
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
    
    uint8_t handleC(Op op)
    {
        return uint8_t(op) | handleC();
    }
    
    uint8_t handleI()
    {
        int32_t i;
        expect(integer(i), Compiler::Error::ExpectedInt);
        expect(i >= 0 && i <= 15, Compiler::Error::ParamOutOfRange);
        return uint8_t(i);
    }
    
    uint8_t handleId()
    {
        std::string id;
        expect(identifier(id), Compiler::Error::ExpectedIdentifier);
        auto it = _symbols.find(id);
        expect(it != _symbols.end(), Compiler::Error::UndefinedIdentifier);
        return it->second;
    }
    
    void handleOpParams(uint8_t a)
    {
        _rom8.push_back(a);
        expectWithoutRetire(Token::NewLine);
    }

    void handleOpParams(uint8_t a, uint8_t b)
    {
        _rom8.push_back(a);
        _rom8.push_back(b);
        expectWithoutRetire(Token::NewLine);
    }

    void handleOpParamsReverse(uint8_t a, uint8_t b)
    {
        _rom8.push_back(b);
        _rom8.push_back(a);
        expectWithoutRetire(Token::NewLine);
    }

    void handleOpParamsRdRs(Op op, uint8_t rd, uint8_t rs)
    {
        _rom8.push_back(uint8_t(op));
        _rom8.push_back((rd << 6) | (rs << 4));
        expectWithoutRetire(Token::NewLine);
    }

    void handleOpParamsRdRs(Op op, uint8_t a, uint8_t rd, uint8_t rs)
    {
        _rom8.push_back(uint8_t(op));
        _rom8.push_back(a);
        _rom8.push_back((rd << 6) | (rs << 4));
        expectWithoutRetire(Token::NewLine);
    }

    void handleOpParamsRdRsSplit(Op op, uint8_t rd, uint8_t a, uint8_t rs)
    {
        _rom8.push_back(uint8_t(op));
        _rom8.push_back(a);
        _rom8.push_back((rd << 6) | (rs << 4));
        expectWithoutRetire(Token::NewLine);
    }

    bool opStatement()
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

    bool forStatement()
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
    
    bool ifStatement()
    {
        if (!match(Reserved::If)) {
            return false;
        }
        expect(Token::NewLine);
        statements();
        
        if (match(Reserved::Else)) {
            expect(Token::NewLine);
            statements();
        }
        
        expect(match(Reserved::End), Compiler::Error::ExpectedEnd);
        return true;
    }
    
    void expectWithoutRetire(Token token)
    {
        if (_scanner.getToken() != token) {
            _expectedToken = token;
            _expectedString = "";
            throw true;
        }
    }
    
    void expect(Token token, const char* str = nullptr)
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
    
    void expect(bool passed, Compiler::Error error)
    {
        if (!passed) {
            _error = error;
            throw true;
        }
    }
    
    bool match(Token token, const char* str = nullptr)
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
    
    bool match(Reserved r)
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
    
    bool identifier(std::string& id)
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
    
    bool opcode()
    {
        return isOpcode(_scanner.getToken(), _scanner.getTokenString());
    }
    
    bool reserved()
    {
        Reserved r;
        return isReserved(_scanner.getToken(), _scanner.getTokenString(), r);
    }
    
    bool reserved(Reserved &r)
    {
        return isReserved(_scanner.getToken(), _scanner.getTokenString(), r);
    }
    
    bool isOpcode(Token token, const std::string str)
    {
        Op op;
        OpParams par;
        return opcode(token, str, op, par);
    }
    
    bool opcode(Token token, const std::string str, Op& op, OpParams& par)
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
    
    bool isReserved(Token token, const std::string str, Reserved& r)
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
    
    bool opDataFromString(const std::string str, OpData& data) const
    {
        auto it = find_if(_opcodes.begin(), _opcodes.end(),
                        [str](const OpData& opData) { return opData._str == str; });
        if (it != _opcodes.end()) {
            data = *it;
            return true;
        }
        return false;
    }
 
    bool opDataFromOp(const Op op, OpData& data) const
    {
        auto it = find_if(_opcodes.begin(), _opcodes.end(),
                        [op](const OpData& opData) { return opData._op == op; });
        if (it != _opcodes.end()) {
            data = *it;
            return true;
        }
        return false;
    }

    Scanner _scanner;
    Compiler::Error _error = Compiler::Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
    
    struct Effect
    {
        Effect(char cmd, uint8_t count)
            : _cmd(cmd)
            , _count(count)
        { }
        
        char _cmd;
        uint8_t _count;
        uint8_t _initAddr = 0;
        uint8_t _loopAddr = 0;
    };
    
    std::map<std::string, uint8_t> _symbols;
    std::vector<Effect> _effects;
    std::vector<uint32_t> _rom32;
    std::vector<uint32_t> _rom8;
    uint32_t _nextMem = 0; // next available location in mem
    
    static std::vector<OpData> _opcodes;
};

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
};

bool Compiler::compile(std::istream* istream, std::vector<uint8_t> executable)
{
    CompileEngine eng(istream);
    
    eng.program();
    _error = eng.error();
    _expectedToken = eng.expectedToken();
    _expectedString = eng.expectedString();
    _lineno = eng.lineno();
    
    if (_error == Error::None) {
        eng.emit(executable);
    }
    
    return _error == Error::None;
}
