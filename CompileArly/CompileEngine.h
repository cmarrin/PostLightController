//
//  CompileEngine.h
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//
// base compiler internals

#pragma once

#include "Compiler.h"
#include "Scanner.h"
#include <cstdint>
#include <istream>
#include <vector>

namespace arly {

class CompileEngine
{
public:
    CompileEngine(std::istream* stream) : _scanner(stream) { }
    
    virtual ~CompileEngine() { }
    
    virtual bool program() = 0;
    
    void emit(std::vector<uint8_t>& executable);

    Compiler::Error error() const { return _error; }
    Token expectedToken() const { return _expectedToken; }
    const std::string& expectedString() const { return _expectedString; }
    uint32_t lineno() const { return _scanner.lineno(); }
    
    static bool opDataFromOp(const Op op, OpData& data);

protected:
    enum class Reserved {
        None,
        Const,
        Table,
        Effect,
        End,
        Init,
        Loop,
        ForEach,
        If,
        Else,
        Float,
        Int,
        R0, R1, R2, R3,
        C0, C1, C2, C3,
        Function,
        Def,
    };
    
    virtual bool statement() = 0;
    virtual bool function() = 0;
    virtual bool table() = 0;

    enum class Type { Float, Int };
    
    bool def();
    bool constant();
    bool effect();
    
    bool type(Type& t);
    
    // Value is returned as an int32_t, but it might be a float
    bool value(int32_t& i, Type);
    
    bool var();
    
    // There are 4 temp values for use by statements. They
    // are allocated, used, then freed when done. There are
    // special LoadTemp, StoreTemp opcodes to access them.
    // Alloc throws if it runs out of temps. free throws if
    // you try to free a temp that hasn't been allocated.
    uint8_t allocTemp();
    void freeTemp(uint8_t i);
    
    // The expect methods validate the passed param and if
    // there is no match, the passed error is saved and
    // throw is called. The first version also retires the
    // current token.
    void expect(Token token, const char* str = nullptr);
    void expect(bool passed, Compiler::Error error);
    void expectWithoutRetire(Token token);
    bool match(Reserved r);
    bool match(Token r);
    void ignoreNewLines();
    
    // These methods check to see if the next token is of the
    // appropriate type. Some versions just return true or
    // false, others also return details about the token
    bool identifier(std::string& id);
    bool integerValue(int32_t& i);
    bool floatValue(float& f);
    bool reserved();
    bool reserved(Reserved &r);
    
    void addOpRdRs(Op op, uint8_t rd, uint8_t rs) { addOpRdRsI(op, rd, rs, 0); }
    void addOpRsI(Op op, uint8_t rs, uint8_t i) { addOpRdRsI(op, 0, rs, i); }
    void addOpRdI(Op op, uint8_t rd, uint8_t i) { addOpRdRsI(op, rd, 0, i); }
    
    void addOpRdRsI(Op op, uint8_t rd, uint8_t rs, uint8_t i)
    {
        _rom8.push_back(uint8_t(op) | (rd << 6) | ((rs & 0x03) << 4) | (i & 0x0f));
    }
    
    virtual bool isReserved(Token token, const std::string str, Reserved& r);

    uint16_t handleFunctionName();

    static bool opDataFromString(const std::string str, OpData& data);

    Compiler::Error _error = Compiler::Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
    
    struct Def
    {
        Def(std::string name, uint8_t value)
            : _name(name)
            , _value(value)
        { }
        std::string _name;
        uint8_t _value;
    };
    
    struct Symbol
    {
        Symbol(std::string name, uint8_t addr, bool rom)
            : _name(name)
            , _addr(addr)
            , _rom(rom)
        { }
        std::string _name;
        uint8_t _addr;
        bool _rom;
    };
    
    struct Function
    {
        Function(std::string name, uint16_t addr)
            : _name(name)
            , _addr(addr)
        { }
        std::string _name;
        uint16_t _addr;
    };
    
    struct Effect
    {
        Effect(char cmd, uint8_t count, uint16_t initAddr, uint16_t loopAddr)
            : _cmd(cmd)
            , _count(count)
            , _initAddr(initAddr)
            , _loopAddr(loopAddr)
        { }
        
        char _cmd;
        uint8_t _count;
        uint16_t _initAddr = 0;
        uint16_t _loopAddr = 0;
    };
    
    Scanner _scanner;

    std::vector<Def> _defs;
    std::vector<Symbol> _symbols;
    std::vector<Function> _functions;
    std::vector<Effect> _effects;
    std::vector<uint32_t> _rom32;
    std::vector<uint8_t> _rom8;
    uint32_t _nextMem = 0; // next available location in mem
    
    uint8_t _tempAllocationMap = 0;
};

}
