//
//  CompileEngine.h
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//
// base compiler internals

#pragma once

#include "Compiler.h"
#include "Interpreter.h"
#include "Scanner.h"
#include <cstdint>
#include <istream>
#include <vector>

namespace arly {

class CompileEngine
{
public:
    CompileEngine(std::istream* stream);
    
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
        Def,
        Struct,
        Const,
        Table,
        Var,
        Function,
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
    };
    
    virtual bool statement() = 0;
    virtual bool function() = 0;
    virtual bool table() = 0;

    enum class Type { None, Float, Int, UInt8, Color };
    
    bool def();
    bool constant();
    bool effect();
    
    bool type(Type& t);
    
    bool values(Type);

    // Value is returned as an int32_t, but it might be a float
    bool value(int32_t& i, Type);
        
    // Every function has local variables on the top of the stack.
    // They are allocated by the 'var' keyword. Functiions can
    // also define temp locations on top of that var storage.
    // The methods below alloc and free those locations. The 
    // allocated temp locations are kept in a 32 bit map, so
    // there can be a max of 32 temps active at one time.
    // There is also a _tempSize value which says how many words
    // of temp storage is being used currently. When allocTemp
    // is called the first free bit in the map is found. If
    // its index is less than _tempSize, this is a reuse of
    // a previously allocated and then freed location. If it
    // is equal then this is a new temp value and _tempSize
    // is incremented. If it is greater, there is an internal
    // error.
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
    
    void addOp(Op op) { _rom8.push_back(uint8_t(op)); }
    void addOpR(Op op, uint8_t r) { _rom8.push_back(uint8_t(op) | (r & 0x03)); }
    void addOpRdRs(Op op, uint8_t rd, uint8_t rs) { addOpRdRsI(op, rd, rs, 0); }
    void addOpRsI(Op op, uint8_t rs, uint8_t i) { addOpRdRsI(op, 0, rs, i); }
    void addOpRdI(Op op, uint8_t rd, uint8_t i) { addOpRdRsI(op, rd, 0, i); }
    
    void addOpRdRsI(Op op, uint8_t rd, uint8_t rs, uint8_t i)
    {
        _rom8.push_back(uint8_t(op));
        _rom8.push_back(uint8_t((rd << 6) | ((rs & 0x03) << 4) | (i & 0x0f)));
    }
    
    void addOpRInt(Op op, uint8_t r, uint8_t i)
    {
        _rom8.push_back(uint8_t(op) | (r & 0x03));
        _rom8.push_back(i);
    }
    
    void addOpId(Op op, uint8_t id) { addOpRInt(op, 0, id); }
    void addOpRId(Op op, uint8_t r, uint8_t id) { addOpRInt(op, r, id); }
    void addOpRConst(Op op, uint8_t r, uint8_t c) { addOpRInt(op, r, c); }
    
    void addOpTarg(Op op, uint16_t targ)
    {
        _rom8.push_back(uint8_t(op) | ((targ >> 6) & 0x03));
        _rom8.push_back(uint8_t(targ));
    }
    
    virtual bool isReserved(Token token, const std::string str, Reserved& r);

    uint16_t handleFunctionName();

    static bool opDataFromString(const std::string str, OpData& data);

    struct Def
    {
        Def(std::string name, uint8_t value)
            : _name(name)
            , _value(value)
        { }
        std::string _name;
        uint8_t _value;
    };
    
    class Symbol
    {
    public:
        enum class Storage { None, Const, Global, Local, Color };
        
        Symbol() { }
        
        Symbol(const std::string& name, uint8_t addr, Type type, Storage storage, uint8_t size = 1)
            : _name(name)
            , _addr(addr)
            , _type(type)
            , _storage(storage)
            , _size(size)
        { }
        
        // Used to add locals to function
        Symbol(const char* name, uint8_t addr, Type type)
            : _name(name)
            , _addr(addr)
            , _type(type)
            , _storage(Storage::Local)
            , _size(1)
        { }

        const std::string& name() const { return _name; }
        uint8_t addr() const;
        Type type() const { return _type; }
        Storage storage() const { return _storage; }
        uint8_t size() const { return _size; }

    private:
        std::string _name;
        uint8_t _addr = 0;
        Type _type = Type::None;
        Storage _storage = Storage::None;
        uint8_t _size = 0;
    };
    
    using SymbolList = std::vector<Symbol>;
    
    class Function
    {
    public:
        Function() { }
        
        Function(const std::string& name, uint16_t addr)
            : _name(name)
            , _addr(addr)
        { }

        // Used to create built-in native functions
        Function(const char* name, Interpreter::NativeFunction native, const SymbolList& locals)
            : _name(name)
            , _native(native)
            , _locals(locals)
        { }

        const std::string& name() const { return _name; }
        uint16_t addr() const { return _addr; }
        Interpreter::NativeFunction native() const { return _native; }
        std::vector<Symbol>& locals() { return _locals; }
        const std::vector<Symbol>& locals() const { return _locals; }
        
        bool isNative() const { return _native != Interpreter::NativeFunction::None; }
        
    private:
        std::string _name;
        uint16_t _addr = 0;
        Interpreter::NativeFunction _native = Interpreter::NativeFunction::None;
        std::vector<Symbol> _locals;        
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
    
    Function& currentFunction()
    {
        if (_functions.empty()) {
            _error = Compiler::Error::InternalError;
            throw true;
        }
        return _functions.back();
    }
    
    std::vector<Symbol>& currentLocals() { return currentFunction().locals(); }

    bool findSymbol(const std::string&, Symbol&);
    bool findFunction(const std::string&, Function&);

    Compiler::Error _error = Compiler::Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
    
    Scanner _scanner;

    std::vector<Def> _defs;
    std::vector<Symbol> _globals;
    std::vector<Function> _functions;
    std::vector<Effect> _effects;
    std::vector<uint32_t> _rom32;
    std::vector<uint8_t> _rom8;
    
    // Vars are defined in 2 places. At global scope (when there are
    // no active functions) they are placed in _global memory.
    // When a function is being defined the vars are placed on the
    // stack.
    
    // These vars deal with stack memory use for the current function.
    // On entry, _nextMem is 0 which means no vars have been defined.
    // It increases on every var statement, according to how many
    // words that var takes. Once all vars are defined (which must
    // be at the top of the function, before any statements) the
    // next memory locations are temps, indicated by _tempSize. Var
    // and temp memory are accessed with LoadLocal/StoreLocal.
    uint16_t _nextMem = 0; // next available location in mem
    uint16_t _localHighWaterMark = 0;
    uint32_t _tempAllocationMap = 0;
    uint8_t _tempSize = 0;
    uint16_t _globalSize = 0;
};

}
