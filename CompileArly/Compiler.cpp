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
        _symbols.emplace(id, _nextRomAddress++);
        _rom.push_back(val);
        assert(_rom.size() == _nextRomAddress);
        
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
        _symbols.emplace(id, _nextRomAddress);
        
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
        
        _effects.emplace(id[0], EffectParams(paramCount, _nextRomAddress));

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
            
            _rom.push_back(val);
            _nextRomAddress++;
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
    
    bool opParam()
    {
        if (match(Token::Identifier)) {
            return true;
        }
        if (match(Token::Integer)) {
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
        expect(Token::Integer);
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
    
    bool opStatement()
    {
        Op opcode;
        OpParams par;
        if (!isOpcode(_scanner.getToken(), _scanner.getTokenString(), opcode, par)) {
            return false;
        }
        
        // Get the params in the sequence specified in OpParams
//        switch(par) {
//            case None:
//        }
        
        opParams();
        return true;
    }

    bool opParams()
    {
        bool haveValues = false;
        while(1) {
            if (!opParam()) {
                break;
            }
            haveValues = true;
        }
        return haveValues;
    }

    bool forStatement()
    {
        if (!match(Reserved::ForEach)) {
            return false;
        }
        expect(Token::Identifier);
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
    
    void expect(bool passed, Compiler::Error error)
    {
        if (!passed) {
            _error = error;
            throw true;
        }
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
        Reserved rr;
        return isReserved(_scanner.getToken(), _scanner.getTokenString(), rr);
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
            { "LoadColorX",     OpData(Op::LoadColor        , OpParams::Rd_Id_Rs) },
            { "LoadX",          OpData(Op::LoadX            , OpParams::Rd_Id_Rs_I) },
            { "StoreColorX",    OpData(Op::StoreColorX      , OpParams::Id_Rd_Rs) },
            { "StoreX",         OpData(Op::StoreX           , OpParams::Id_Rd_I_Rs) },
            { "MoveColor",      OpData(Op::MoveColor        , OpParams::Rd_Rs) },
            { "Move",           OpData(Op::Move             , OpParams::Rd_Rs) },
            { "LoadVal",        OpData(Op::LoadVal          , OpParams::Rd_Rs) },
            { "StoreVal",       OpData(Op::StoreVal         , OpParams::Rd_Rs) },
            { "MinInt",         OpData(Op::MinInt           , OpParams::None) },
            { "MinFloat",       OpData(Op::MinFloat         , OpParams::None) },
            { "MaxInt",         OpData(Op::MaxInt           , OpParams::None) },
            { "MaxFloat",       OpData(Op::MaxFloat         , OpParams::None) },
            { "SetLight",       OpData(Op::SetLight         , OpParams::Rd_Rs) },
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
            { "LoadColorParam", OpData(Op::LoadColorParam   , OpParams::R_I) },
            { "LoadIntParam",   OpData(Op::LoadIntParam     , OpParams::R_I) },
            { "LoadFloatParam", OpData(Op::LoadFloatParam   , OpParams::R_I) },
            { "LoadColor",      OpData(Op::LoadColor        , OpParams::R_Id) },
            { "Load",           OpData(Op::Load             , OpParams::R_Id) },
            { "StoreColor",     OpData(Op::StoreColor       , OpParams::Id_R) },
            { "Store",          OpData(Op::Store            , OpParams::Id_R) },
            { "LoadBlack",      OpData(Op::LoadBlack        , OpParams::R) },
            { "LoadZero",       OpData(Op::LoadZero         , OpParams::R) },
            { "LoadIntOne",     OpData(Op::LoadIntOne       , OpParams::R) },
            { "LoadFloatOne",   OpData(Op::LoadFloatOne     , OpParams::R) },
            { "LoadByteMax",    OpData(Op::LoadByteMax      , OpParams::R) },
            { "Return",         OpData(Op::Return           , OpParams::R) },
            { "ToFloat",        OpData(Op::ToFloat          , OpParams::R) },
            { "ToInt",          OpData(Op::ToInt            , OpParams::R) },
            { "SetAllLights",   OpData(Op::SetAllLights     , OpParams::R) },
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
    std::vector<uint32_t> _rom;
    uint8_t _nextRomAddress = 0;
    
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

