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
    'struct' <id> '{' { structEntry } '}';
    
// First integer is num elements, second is size of each element
var:
    'var' type <id> [ '*' ] [ <integer> ]

function:
    'function' <id> '( formalParameterList ')' '{' { statement } '}' ;

effect:
    'effect' <id> <integer> <id> <id> ;

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
    | expressionStatement
    ;
  
compoundStatement:
    '{' { statement } '}' ;

ifStatement:
    'if' '(' arithmeticExpression ')' statement ['else' statement ] ;

forStatement:
    'foreach' '(' identifier ':' arithmeticExpression ')' statement ;
    
expressionStatement:
    arithmeticExpression ';' ;
    
arithmeticExpression:
      unaryExpression
    | unaryExpression operator arithmeticExpression

unaryExpression:
      postfixExpression
    | '++' unaryExpression
    | '--' unaryExpression
    | '-' unaryExpression
    | '~' unaryExpression
    | '!' unaryExpression
    | '&' unaryExpression
    ;

postfixExpression:
      primaryExpression
    | primaryExpression '++'
    | primaryExpression '--'
    | primaryExpression '(' argumentList ')'
    | primaryExpression '[' arithmeticExpression ']'
    | primaryExpression '.' identifier
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
  	CloverCompileEngine(std::istream* stream) : CompileEngine(stream) { }
  	
    virtual bool program() override;

protected:
    virtual bool statement() override;
    virtual bool function() override;
    virtual bool table() override;
  
private:
    class OperatorInfo {
    public:
        enum class Assoc { Left = 0, Right = 1 };
        
        OperatorInfo() { }
        OperatorInfo(Token token, uint8_t prec, Assoc assoc, bool sto, Op op)
        {
            OperatorInfo info;
            info._token = static_cast<uint32_t>(token);
            info._prec = prec;
            info._op = static_cast<uint32_t>(op);
            info._assoc = static_cast<uint32_t>(assoc);
            info._sto = sto;
            _u = info._u;
        }
        
        bool operator==(const Token& t)
        {
            return static_cast<Token>(_token) == t;
        }
        
        Token token() const { return static_cast<Token>(_token); }
        uint8_t prec() const { return _prec; }
        Op op() const { return static_cast<Op>(_op); }
        bool sto() const { return _sto != 0; }
        Assoc assoc() const { return static_cast<Assoc>(_assoc); }

    private:
        union {
            struct {
                uint32_t _token : 16;
                uint32_t _op : 8;
                uint32_t _prec : 5;
                uint32_t _assoc : 1;
                uint32_t _sto : 1;
            };
            uint32_t _u;
        };
    };
    
    // OperatorInfo is in Flash, so we need to access it as a single 4 byte read
    static_assert(sizeof(OperatorInfo) == 4, "OperatorInfo must fit in 4 bytes");

    bool element();
    bool strucT();
    
    bool structEntry();

    bool compoundStatement();
    bool ifStatement();
    bool forStatement();
    bool expressionStatement();
    
    bool arithmeticExpression(uint8_t minPrec = 1);
    bool unaryExpression();
    bool postfixExpression();
    bool primaryExpression();

    bool formalParameterList();
    bool argumentList();
    
    bool operatorInfo(Token token, OperatorInfo&) const;

    virtual bool isReserved(Token token, const std::string str, Reserved&) override;

    struct ParamEntry
    {
        ParamEntry(const std::string& name, Type type)
            : _name(name)
            , _type(type)
        { }
        std::string _name;
        Type _type;
    };
    
    struct Struct
    {
        Struct(const std::string& name)
            : _name(name)
        { }
        std::string _name;
        std::vector<ParamEntry> _entries;
    };
    

    struct FunctionParams
    {
        FunctionParams(const std::string& name)
            : _name(name)
        { }
        std::string _name;
        std::vector<ParamEntry> _entries;
    };
    
    std::vector<Struct> _structs;
    std::vector<FunctionParams> _functionParams;
};

}
