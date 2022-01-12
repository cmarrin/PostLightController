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
        
        _effects.emplace(id[0], EffectParams(paramCount, _rom32.size()));

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

        // FIXME: need to "allocate" size
        _symbols.emplace(id, 0);

        return true;
    }

    bool init()
    {
        if (!match(Reserved::Init)) {
            return false;
        }
        expect(Token::NewLine);
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
        if (!isOpcode(_scanner.getToken(), _scanner.getTokenString(), op, par)) {
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
        statements();
        expect(match(Reserved::End), Compiler::Error::ExpectedEnd);
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
        return isOpcode(token, str, op, par);
    }
    
    bool isOpcode(Token token, const std::string str, Op& op, OpParams& par)
    {
        static std::map<std::string, OpData> opcodes = {
            { "LoadColorX",     OpData(Op::LoadColor        , OpParams::Cd_Id_Rs) },
            { "LoadX",          OpData(Op::LoadX            , OpParams::Rd_Id_Rs_I) },
            { "StoreColorX",    OpData(Op::StoreColorX      , OpParams::Id_Rd_Cs) },
            { "StoreX",         OpData(Op::StoreX           , OpParams::Id_Rd_I_Rs) },
            { "MoveColor",      OpData(Op::MoveColor        , OpParams::Cd_Cs) },
            { "Move",           OpData(Op::Move             , OpParams::Rd_Rs) },
            { "LoadVal",        OpData(Op::LoadVal          , OpParams::Rd_Cs) },
            { "StoreVal",       OpData(Op::StoreVal         , OpParams::Cd_Rs) },
            { "MinInt",         OpData(Op::MinInt           , OpParams::None) },
            { "MinFloat",       OpData(Op::MinFloat         , OpParams::None) },
            { "MaxInt",         OpData(Op::MaxInt           , OpParams::None) },
            { "MaxFloat",       OpData(Op::MaxFloat         , OpParams::None) },
            { "SetLight",       OpData(Op::SetLight         , OpParams::Rd_Cs) },
            { "Init",           OpData(Op::Init             , OpParams::Id) },
            { "Random",         OpData(Op::Random           , OpParams::None) },
            { "Bor",            OpData(Op::Bor              , OpParams::None) },
            { "Bxor",           OpData(Op::Bxor             , OpParams::None) },
            { "Band",           OpData(Op::Band             , OpParams::None) },
            { "Bnot",           OpData(Op::Bnot             , OpParams::None) },
            { "Or",             OpData(Op::Or               , OpParams::None) },
            { "And",            OpData(Op::And              , OpParams::None) },
            { "Not",            OpData(Op::Not              , OpParams::None) },
            { "LTInt",          OpData(Op::LTInt            , OpParams::None) },
            { "LTFloat",        OpData(Op::LTFloat          , OpParams::None) },
            { "LEInt",          OpData(Op::LEInt            , OpParams::None) },
            { "LEFloat",        OpData(Op::LEFloat          , OpParams::None) },
            { "EQInt",          OpData(Op::EQInt            , OpParams::None) },
            { "EQFloat",        OpData(Op::EQFloat          , OpParams::None) },
            { "NEInt",          OpData(Op::NEInt            , OpParams::None) },
            { "NEFloat",        OpData(Op::NEFloat          , OpParams::None) },
            { "GEInt",          OpData(Op::GEInt            , OpParams::None) },
            { "GEFloat",        OpData(Op::GEFloat          , OpParams::None) },
            { "GTInt",          OpData(Op::GTInt            , OpParams::None) },
            { "GTFloat",        OpData(Op::GTFloat          , OpParams::None) },
            { "AddInt",         OpData(Op::AddInt           , OpParams::None) },
            { "AddFloat",       OpData(Op::AddFloat         , OpParams::None) },
            { "SubInt",         OpData(Op::SubInt           , OpParams::None) },
            { "SubFloat",       OpData(Op::SubFloat         , OpParams::None) },
            { "MulInt",         OpData(Op::MulInt           , OpParams::None) },
            { "MulFloat",       OpData(Op::MulFloat         , OpParams::None) },
            { "DivInt",         OpData(Op::DivInt           , OpParams::None) },
            { "DivFloat",       OpData(Op::DivFloat         , OpParams::None) },
            { "NegInt",         OpData(Op::NegInt           , OpParams::None) },
            { "NegFloat",       OpData(Op::NegFloat         , OpParams::None) },
            { "LoadColorParam", OpData(Op::LoadColorParam   , OpParams::C_I) },
            { "LoadIntParam",   OpData(Op::LoadIntParam     , OpParams::R_I) },
            { "LoadFloatParam", OpData(Op::LoadFloatParam   , OpParams::R_I) },
            { "LoadColor",      OpData(Op::LoadColor        , OpParams::C_Id) },
            { "Load",           OpData(Op::Load             , OpParams::R_Id) },
            { "StoreColor",     OpData(Op::StoreColor       , OpParams::Id_C) },
            { "Store",          OpData(Op::Store            , OpParams::Id_R) },
            { "LoadBlack",      OpData(Op::LoadBlack        , OpParams::C) },
            { "LoadZero",       OpData(Op::LoadZero         , OpParams::R) },
            { "LoadIntOne",     OpData(Op::LoadIntOne       , OpParams::R) },
            { "LoadFloatOne",   OpData(Op::LoadFloatOne     , OpParams::R) },
            { "LoadByteMax",    OpData(Op::LoadByteMax      , OpParams::R) },
            { "Return",         OpData(Op::Return           , OpParams::R) },
            { "ToFloat",        OpData(Op::ToFloat          , OpParams::R) },
            { "ToInt",          OpData(Op::ToInt            , OpParams::R) },
            { "SetAllLights",   OpData(Op::SetAllLights     , OpParams::C) },
        };
    
        if (token != Token::Identifier) {
            return false;
        }
        
        auto it = opcodes.find(str);
        if (it != opcodes.end()) {
            op = it->second._op;
            par = it->second._par;
            return true;
        }
        return false;
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
    
    Scanner _scanner;
    Compiler::Error _error = Compiler::Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
    
    struct EffectParams
    {
        EffectParams(uint8_t count, uint8_t addr) : _count(count), _addr(addr) { }
        uint8_t _count, _addr;
    };
    
    std::map<std::string, uint8_t> _symbols;
    std::map<char, EffectParams> _effects;
    std::vector<uint32_t> _rom32;
    std::vector<uint32_t> _rom8;
};

bool Compiler::compile(std::istream* stream)
{
    CompileEngine eng(stream);
    
    eng.program();
    _error = eng.error();
    _expectedToken = eng.expectedToken();
    _expectedString = eng.expectedString();
    _lineno = eng.lineno();
    
    return false;
}

