//
//  CloverCompileEngine.h
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//
// Clover compiler
//
// A simple imperative language which generates code that can be 
// executed by the Arly Interpreter
//

#pragma once

#include "Compiler.h"
#include "CompileEngine.h"
#include "Opcodes.h"
#include "Scanner.h"
#include <cstdint>
#include <istream>
#include <variant>

namespace arly {

//////////////////////////////////////////////////////////////////////////////
//
//  Class: CompileEngine
//
//////////////////////////////////////////////////////////////////////////////

/*

BNF:

program:
    { element } ;

element:
    def | constant | table | struct | var | function | effect ;
    
def:
    'def' <id> <integer> ';'

constant:
    'const' type <id> value ';' ;
    
table:
    'table' type <id> '{' values '}' ;

struct:
    'struct' <id> '{' { structEntry } '}' ;
    
// First integer is num elements, second is size of each element
var:
    'var' type [ '*' ] <id> [ <integer> ] ';' ;

function:
    'function' [ <type> ] <id> '( formalParameterList ')' '{' { var } { statement } '}' ;

effect:
    'effect' <id> <integer> <id> <id> ';' ;

structEntry:
    type <id> ';' ;

// <id> is a struct name
type:
    'float' | 'int' | <id> 

value:
    ['-'] <float> | ['-'] <integer>

statement:
      compoundStatement
    | ifStatement
    | forStatement
    | returnStatement
    | expressionStatement
    ;
  
compoundStatement:
    '{' { statement } '}' ;

ifStatement:
    'if' '(' arithmeticExpression ')' statement ['else' statement ] ;

forStatement:
    'foreach' '(' identifier ':' arithmeticExpression ')' statement ;
    
returnStatement:
      'return' [ arithmeticExpression ] ';' ;
      
expressionStatement:
    arithmeticExpression ';' ;
    
arithmeticExpression:
      unaryExpression
    | unaryExpression operator arithmeticExpression

unaryExpression:
      postfixExpression
    | '-' unaryExpression
    | '~' unaryExpression
    | '!' unaryExpression
    | '&' unaryExpression
    ;

postfixExpression:
      primaryExpression
    | postfixExpression '(' argumentList ')'
    | postfixExpression '[' arithmeticExpression ']'
    | postfixExpression '.' identifier
    ;

primaryExpression:
      '(' arithmeticExpression ')'
    | <id>
    | <float>
    | <integer>
    ;
    
formalParameterList:
      (* empty *)
    | type identifier { ',' type identifier }
    ;

argumentList:
        (* empty *)
      | arithmeticExpression { ',' arithmeticExpression }
      ;

operator: (* operator   precedence   association *)
               '='     (*   1          Right    *)
    |          '+='    (*   1          Right    *)
    |          '-='    (*   1          Right    *)
    |          '*='    (*   1          Right    *)
    |          '/='    (*   1          Right    *)
    |          '&='    (*   1          Right    *)
    |          '|='    (*   1          Right    *)
    |          '^='    (*   1          Right    *)
    |          '||'    (*   2          Left     *)
    |          '&&'    (*   3          Left     *)
    |          '|'     (*   4          Left     *)
    |          '^'     (*   5          Left     *)
    |          '&'     (*   6          Left     *)
    |          '=='    (*   7          Left     *)
    |          '!='    (*   7          Left     *)
    |          '<'     (*   8          Left     *)
    |          '>'     (*   8          Left     *)
    |          '>='    (*   8          Left     *)
    |          '<='    (*   8          Left     *)
    |          '+'     (*   10         Left     *)
    |          '-'     (*   10         Left     *)
    |          '*'     (*   11         Left     *)
    |          '/'     (*   11         Left     *)
    ;
    
*/

class CloverCompileEngine : public CompileEngine {
public:
  	CloverCompileEngine(std::istream* stream)
        : CompileEngine(stream)
    { }
  	
    virtual bool program() override;

protected:
    virtual bool statement() override;
    virtual bool function() override;
    virtual bool table() override;
    virtual bool type(Type&) override;
  
    bool var();

private:
    class OpInfo {
    public:        
        OpInfo() { }
        
        enum class Assign { None, Only, Op };
        
        // assign says this is an assignmentOperator, opAssign says it also has a binary op
        OpInfo(Token token, uint8_t prec, Op intOp, Op floatOp, Assign assign)
            : _token(token)
            , _intOp(intOp)
            , _floatOp(floatOp)
            , _prec(prec)
            , _assign(assign)
        {
        }
        
        bool operator==(const Token& t)
        {
            return static_cast<Token>(_token) == t;
        }
        
        Token token() const { return _token; }
        uint8_t prec() const { return _prec; }
        Op intOp() const { return _intOp; }
        Op floatOp() const { return _floatOp; }
        Assign assign() const { return _assign; }

    private:
        Token _token;
        Op _intOp;
        Op _floatOp;
        uint8_t _prec;
        Assign _assign;
    };
    
    bool element();
    bool strucT();
    
    bool structEntry();

    bool compoundStatement();
    bool ifStatement();
    bool forStatement();
    bool returnStatement();
    bool expressionStatement();
    
    enum class ArithType { Assign, Op };
    bool assignmentExpression() { return arithmeticExpression(1, ArithType::Assign); }
    bool arithmeticExpression(uint8_t minPrec = 1, ArithType = ArithType::Op);
    bool unaryExpression();
    bool postfixExpression();
    bool primaryExpression();

    bool formalParameterList();
    bool argumentList(const Function& fun);
    
    bool opInfo(Token token, OpInfo&) const;

    virtual bool isReserved(Token token, const std::string str, Reserved&) override;

    uint8_t findInt(int32_t);
    uint8_t findFloat(float);
    
    // The ExprStack
    //
    // This is a stack of the operators being processed. Values can be:
    //
    //      Id      - string id
    //      Float   - float constant
    //      Int     - int32_t constant
    //      Dot     - r0 contains a ref. entry is an index into a Struct.

    // ExprAction indicates what to do with the top entry on the ExprStack during baking
    //
    //      Right       - Entry is a RHS, so it can be a float, int or id and the value 
    //                    is loaded into r0
    //      Left        - Entry is a LHS, so it must be an id, value in r0 is stored at the id
    //      Function    - Entry is the named function which has already been emitted so value
    //                    is the return value in r0
    //      Ref         - Value left in r0 must be an address to a value (Const or RAM)
    //      Deref       = Value must be a Struct entry for the value in _stackEntry - 1
    //      Dot         - Dot operator. TOS must be a struct id, TOS-1 must be a ref with
    //                    a type. Struct id must be a member of the type of the ref.
    //                    pop the two 
    //
    enum class ExprAction { Left, Right, Ref, Index, Offset };
    Type bakeExpr(ExprAction);
    bool isExprFunction();
    uint8_t elementSize(const Symbol&);
    
    struct ParamEntry
    {
        ParamEntry(const std::string& name, Type type)
            : _name(name)
            , _type(type)
        { }
        std::string _name;
        Type _type;
    };
    
    class Struct
    {
    public:
        Struct(const std::string& name)
            : _name(name)
        { }
        
        void addEntry(const std::string& name, Type type)
        {
            _entries.emplace_back(name, type);
            
            // FIXME: For now assume all 1 word types. Will we support Structs in Structs?
            // FIXME: Max size is 16, check that
            _size++;
        }
        
        const std::vector<ParamEntry>& entries() const { return _entries; }
        
        const std::string& name() const { return _name; }
        uint8_t size() const { return _size; }
        
    private:
        std::string _name;
        std::vector<ParamEntry> _entries;
        uint8_t _size = 0;
    };

    class ExprEntry
    {
    public:
        struct Ref
        {
            Ref(Type type) : _type(type) { }
            
            Type _type;
        };
        
        struct Function
        {
            Function(const std::string& s) : _name(s) { }
            std::string _name;
        };
        
        struct Dot
        {
            Dot(uint8_t index) : _index(index) { }
            
            uint8_t _index;
        };
        
        struct Value
        {
            Value(Type type) : _type(type) { }
            
            Type _type;
        };
        
        enum class Type {
            None = 0,
            Id = 1,
            Float = 2, 
            Int = 3, 
            Ref = 4, 
            Function = 5, 
            Dot = 6, 
            Value = 7
        };
        
        ExprEntry() { _variant = std::monostate(); }
        ExprEntry(const std::string& s) { _variant = s; }
        ExprEntry(float f) { _variant = f; }
        ExprEntry(int32_t i) { _variant = i; }
        ExprEntry(const Ref& ref) { _variant = ref; }
        ExprEntry(const Function& fun) { _variant = fun; }
        ExprEntry(const Dot& dot) { _variant = dot; }
        ExprEntry(const Value& val) { _variant = val; }
                
        operator const std::string&() const { return std::get<std::string>(_variant); }
        operator float() const { return std::get<float>(_variant); }
        operator int32_t() const { return std::get<int32_t>(_variant); }
        operator const Ref&() const { return std::get<Ref>(_variant); }
        operator const Dot&() const { return std::get<Dot>(_variant); }
        operator const Value&() const { return std::get<Value>(_variant); }
        
        Type type() const { return Type(_variant.index()); }

    private:
        std::variant<std::monostate
                     , std::string
                     , float
                     , int32_t
                     , Ref
                     , Function
                     , Dot
                     , Value
                     > _variant;
    };

    const Struct& structFromType(Type);
    uint8_t findStructElementId(Type, const std::string& id);

    std::vector<Struct> _structs;
    std::vector<ExprEntry> _exprStack;
    
    // built-in variables. These can be color registers, functions, arrays, etc.
    std::vector<Symbol> _builtins;
};

}
