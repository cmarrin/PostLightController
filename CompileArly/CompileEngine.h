//
//  CompileEngine.h
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//
// Arly compiler internals

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
    
    bool program();
    
    void emit(std::vector<uint8_t>& executable);

    bool opcode(Op op, std::string& str, OpParams& par);
    
    Compiler::Error error() const { return _error; }
    Token expectedToken() const { return _expectedToken; }
    const std::string& expectedString() const { return _expectedString; }
    uint32_t lineno() const { return _scanner.lineno(); }
    
    static bool opDataFromOp(const Op op, OpData& data);

private:
    enum class Type { Float, Int };
    
    void constants();
    void tables();
    void effects();
    bool constant();
    bool table();
    bool effect();
    
    bool type(Type& t);
    
    void tableEntries();
    
    bool values();

    // Value is returned as an int32_t, but it might be a float
    bool value(int32_t& i);
    
    void defs();
    bool def();

    bool init();
    bool loop();
    void statements();
    bool statement();
    
    uint8_t handleR();
    uint8_t handleR(Op op);
    uint8_t handleC();
    uint8_t handleC(Op op);
    uint8_t handleI();
    uint8_t handleId();
    
    void handleOpParams(uint8_t a);
    void handleOpParams(uint8_t a, uint8_t b);
    void handleOpParamsReverse(uint8_t a, uint8_t b);
    void handleOpParamsRdRs(Op op, uint8_t rd, uint8_t rs);
    void handleOpParamsRdRs(Op op, uint8_t a, uint8_t rd, uint8_t rs);
    void handleOpParamsRdRsSplit(Op op, uint8_t rd, uint8_t a, uint8_t rs);
    
    bool opStatement();
    bool forStatement();
    bool ifStatement();
    
    // The expect methods validate the passed param and if
    // there is no match, the passed error is saved and
    // throw is called. The first version also retires the
    // current token.
    void expect(Token token, const char* str = nullptr);
    void expect(bool passed, Compiler::Error error);
    void expectWithoutRetire(Token token);
    bool match(Reserved r);    
    
    // These methods check to see if the next token is of the
    // appropriate type. Some versions just return true or
    // false, others also return details about the token
    bool identifier(std::string& id);
    bool integerValue(int32_t& i);
    bool floatValue(float& f);
    bool opcode();
    bool reserved();
    bool reserved(Reserved &r);
    bool opcode(Token token, const std::string str, Op& op, OpParams& par);
    bool opcode(Token token, const std::string str);
    
    // These methods search the list of reserved words or opcodes
    // to find a match
    bool isReserved(Token token, const std::string str, Reserved& r);
    
    static bool opDataFromString(const std::string str, OpData& data);

    Scanner _scanner;
    Compiler::Error _error = Compiler::Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
    
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
    
    struct Effect
    {
        Effect(char cmd, uint8_t count)
            : _cmd(cmd)
            , _count(count)
        { }
        
        char _cmd;
        uint8_t _count;
        uint16_t _initAddr = 0;
        uint16_t _loopAddr = 0;
    };
    
    std::vector<Symbol> _symbols;
    std::vector<Effect> _effects;
    std::vector<uint32_t> _rom32;
    std::vector<uint8_t> _rom8;
    uint32_t _nextMem = 0; // next available location in mem
    
    static std::vector<OpData> _opcodes;
};

}
