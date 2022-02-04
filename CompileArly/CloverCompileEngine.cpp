/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "CloverCompileEngine.h"

#include "Interpreter.h"

#include <map>
#include <vector>

using namespace arly;

bool
CloverCompileEngine::operatorInfo(Token token, OperatorInfo& op) const
{
    static std::vector<OperatorInfo> opInfo = {
        { Token::Equal,       1, OperatorInfo::Assoc::Right, false, Op::Store },
        { Token::AddSto,    1, OperatorInfo::Assoc::Right, true,  Op::AddInt  },
        { Token::SubSto,    1, OperatorInfo::Assoc::Right, true,  Op::SubInt  },
        { Token::MulSto,    1, OperatorInfo::Assoc::Right, true,  Op::MulInt  },
        { Token::DivSto,    1, OperatorInfo::Assoc::Right, true,  Op::DivInt  },
        { Token::AndSto,    1, OperatorInfo::Assoc::Right, true,  Op::And  },
        { Token::OrSto,     1, OperatorInfo::Assoc::Right, true,  Op::Or   },
        { Token::XorSto,    1, OperatorInfo::Assoc::Right, true,  Op::Xor  },
        { Token::LOr,       6, OperatorInfo::Assoc::Left,  false, Op::LOr  },
        { Token::LAnd,      7, OperatorInfo::Assoc::Left,  false, Op::LAnd },
        { Token::Or,        8, OperatorInfo::Assoc::Left,  false, Op::Or   },
        { Token::Xor,       9, OperatorInfo::Assoc::Left,  false, Op::Xor  },
        { Token::And,      10, OperatorInfo::Assoc::Left,  false, Op::And  },
        { Token::EQ,       11, OperatorInfo::Assoc::Left,  false, Op::EQInt },
        { Token::NE,       11, OperatorInfo::Assoc::Left,  false, Op::NEInt   },
        { Token::LT,       12, OperatorInfo::Assoc::Left,  false, Op::LTInt   },
        { Token::GT,       12, OperatorInfo::Assoc::Left,  false, Op::GTInt   },
        { Token::GE,       12, OperatorInfo::Assoc::Left,  false, Op::GEInt   },
        { Token::LE,       12, OperatorInfo::Assoc::Left,  false, Op::LEInt   },
        { Token::Plus,     14, OperatorInfo::Assoc::Left,  false, Op::AddInt  },
        { Token::Minus,    14, OperatorInfo::Assoc::Left,  false, Op::SubInt  },
        { Token::Mul,      15, OperatorInfo::Assoc::Left,  false, Op::MulInt  },
        { Token::Div,      15, OperatorInfo::Assoc::Left,  false, Op::DivInt  },
    };

    auto it = find_if(opInfo.begin(), opInfo.end(),
                    [token](const OperatorInfo& op) { return op.token() == token; });
    if (it != opInfo.end()) {
        op = *it;
        return true;
    }
    return false;
}

bool
CloverCompileEngine::program()
{
    _scanner.setIgnoreNewlines(true);
    
    try {
        while(element()) { }
    }
    catch(...) {
        return false;
    }
    
    return _error == Compiler::Error::None;
}

bool
CloverCompileEngine::element()
{
    if (def()) {
        expect(Token::Semicolon);
        return true;
    }
    
    if (constant()) {
        expect(Token::Semicolon);
        return true;
    }
    
    if (var()) {
        expect(Token::Semicolon);
        return true;
    }
    
    if (table()) return true;
    if (strucT()) return true;
    if (function()) return true;
    if (effect()) return true;
    return false;
}

bool
CloverCompileEngine::table()
{
    if (!match(Reserved::Table)) {
        return false;
    }
    
    Type t;
    std::string id;
    expect(type(t), Compiler::Error::ExpectedType);
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    expect(Token::OpenBrace);
    
    // Set the start address of the table. tableEntries() will fill them in
    _symbols.emplace_back(id, _rom32.size(), t, Symbol::Storage::Const);
    
    values(t);
    expect(Token::CloseBrace);
    return true;
}

bool
CloverCompileEngine::strucT()
{
    if (!match(Reserved::Struct)) {
        return false;
    }
    
    std::string id;
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);

    // Add a struct entry
    _structs.emplace_back(id);
    
    expect(Token::OpenBrace);
    
    while(structEntry()) { }
    
    expect(Token::CloseBrace);
    return true;
}

bool
CloverCompileEngine::var()
{
    if (!match(Reserved::Var)) {
        return false;
    }

    Type t;
    std::string id;
    
    if (!type(t)) {
        return false;
    }
    
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    
    int32_t size;
    if (!integerValue(size)) {
        size = 1;
    }

    // FIXME: Deal with locals, _nextMem and _globalSize
    _symbols.emplace_back(id, _nextMem, t, Symbol::Storage::Global);
    _nextMem += size;

    // There is only enough room for GlobalSize values
    expect(_nextMem <= GlobalSize, Compiler::Error::TooManyVars);
    
    _globalSize = _nextMem;

    return true;
}

bool
CloverCompileEngine::function()
{
    if (!match(Reserved::Function)) {
        return false;
    }
    
    _nextMem = 0;
    
    std::string id;
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);

    // Remember the function
    _functions.emplace_back(id, uint16_t(_rom8.size()));
    _locals.clear();
    
    expect(Token::OpenParen);
    
    expect(formalParameterList(), Compiler::Error::ExpectedFormalParams);
    
    expect(Token::CloseParen);
    expect(Token::OpenBrace);

    while(var()) { }
    while(statement()) { }

    expect(Token::CloseBrace);

    // Set the high water mark
    if (_nextMem > _localHighWaterMark) {
        _localHighWaterMark = _nextMem;
    }
    
    // Emit Return at the end, just in case
    addOpR(Op::LoadZero, 0);
    addOp(Op::Return);
    
    return true;
}

bool
CloverCompileEngine::structEntry()
{
    Type t;
    std::string id;
    
    if (!type(t)) {
        return false;
    }
    
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    
    expect(Token::Semicolon);
    
    _structs.back()._entries.emplace_back(id, t);
    return true;
}

bool
CloverCompileEngine::statement()
{
    if (compoundStatement()) return true;
    if (ifStatement()) return true;
    if (forStatement()) return true;
    if (expressionStatement()) return true;
    return false;
}

bool
CloverCompileEngine::compoundStatement()
{
    if (!match(Token::OpenBrace)) {
        return false;
    }

    while(statement()) { }

    expect(Token::CloseBrace);
    return true;
}

bool
CloverCompileEngine::ifStatement()
{
    if (!match(Reserved::If)) {
        return false;
    }
    
    expect(Token::OpenParen);
    
    arithmeticExpression();
    
    expect(Token::CloseParen);

    // At this point the expresssion has been executed and the result is in r0
    // Emit the if test
    _rom8.push_back(uint8_t(Op::If));

    // Output a placeholder for sz and rember where it is
    auto szIndex = _rom8.size();
    _rom8.push_back(0);
    
    statement();

    // Update sz
    auto offset = _rom8.size() - szIndex - 1;
    expect(offset < 256, Compiler::Error::ForEachTooBig);
    _rom8[szIndex] = uint8_t(offset);
    
    if (match(Reserved::Else)) {
        _rom8.push_back(uint8_t(Op::Else));

        // Output a placeholder for sz and rember where it is
        auto szIndex = _rom8.size();
        _rom8.push_back(0);

        statement();

        // Update sz
        // rom is pointing at inst past 'end', we want to point at end
        auto offset = _rom8.size() - szIndex - 1;
        expect(offset < 256, Compiler::Error::ForEachTooBig);
        _rom8[szIndex] = uint8_t(offset);
    }
    
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

bool
CloverCompileEngine::forStatement()
{
    if (!match(Reserved::ForEach)) {
        return false;
    }

    expect(Token::OpenParen);

    std::string id;
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);

    expect(Token::Colon);

    arithmeticExpression();
    
    // At this point id has the iterator and r0 has the result
    // of the expression, which is the end value. Save the end
    // value in a temp
    uint8_t temp = allocTemp();
    addOpRdI(Op::Store, 0, temp);
    
    expect(Token::CloseParen);
    
    // Output a placeholder for sz and rember where it is
    auto szIndex = _rom8.size();
    _rom8.push_back(0);
    
    statement();
    
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
CloverCompileEngine::expressionStatement()
{
    if (!arithmeticExpression()) {
        return false;
    }
    
    // Discard result of expression
    //_parser->discardResult();
    
    expect(Token::Semicolon);
    return true;
}

bool
CloverCompileEngine::arithmeticExpression(uint8_t minPrec)
{
    if (!unaryExpression()) {
        return false;
    }
    
    while(1) {
        OperatorInfo opInfo;
        if (!operatorInfo(_scanner.getToken(), opInfo)) {
            break;
        }

        uint8_t nextMinPrec = (opInfo.assoc() == OperatorInfo::Assoc::Left) ? (opInfo.prec() + 1) : opInfo.prec();
        _scanner.retireToken();
        if (opInfo.sto()) {
            // FIXME: Do something
        }
        
        expect(arithmeticExpression(nextMinPrec), Compiler::Error::ExpectedExpr);
    
        switch(opInfo.op()) {
            case Op::Store:
                // Put RHS in r0
                emitRHS();
                
                // FIXME: For now we only handle simple Store case so _exprStack must have id
                expect(_exprStack.back().type() == ExprEntry::Type::Id, Compiler::Error::ExpectedIdentifier);
                
                addOpRId(Op::Store, 0, addrFromId(_exprStack.back()));
                break;
            default: break;
        }
        
        // FIXME: Emit a binary op here with two operands (from a stack?)
        // and the opcode in it->op(). How do we deal with Int vs Float ops?
        
        if (opInfo.sto()) {
            // FIXME: Do something
        }
    }
    return true;
}

bool
CloverCompileEngine::unaryExpression()
{
    if (postfixExpression()) {
        return true;
    }

    Op op = Op::None;
    
    if (match(Token::Inc)) {
        op = Op::IncInt;
    } else if (match(Token::Dec)) {
        op = Op::DecInt;
    } else if (match(Token::Minus)) {
        op = Op::NegInt;
    } else if (match(Token::Twiddle)) {
        op = Op::Not;
    } else if (match(Token::Bang)) {
        op = Op::LNot;
    }

    if (op == Op::None) {
        return false;
    }
    
    expect(unaryExpression(), Compiler::Error::ExpectedExpr);
    unaryExpression();
    
    // FIXME: Emit unary op. Apply op to result from unaryExpression()

    return true;
}

bool
CloverCompileEngine::postfixExpression()
{
    if (!primaryExpression()) {
        return false;
    }
    
    if (match(Token::Inc)) {
        // FIXME: increment result of primaryExpression above
    } else if (match(Token::Dec)) {
        // FIXME: decrement result of primaryExpression above
    } else if (match(Token::OpenParen)) {
        expect(argumentList(), Compiler::Error::ExpectedArgList);
        expect(Token::CloseParen);
        
        // FIXME: The primaryExpression is a function call with the
        // argumentList. Emit that
    } else if (match(Token::OpenBracket)) {
        expect(arithmeticExpression(), Compiler::Error::ExpectedExpr);
        expect(Token::CloseBracket);
        
        // FIXME: The primaryExpression is a deref using the 
        // arithmeticExpression. Emit that
    } else if (match(Token::Dot)) {
        std::string id;
        expect(identifier(id), Compiler::Error::ExpectedIdentifier);
        
        // FIXME: The primaryExpression is an address. The identifier is
        // a struct member. Use its offset in a deref op (load or store?)
    }
    return true;
}

bool
CloverCompileEngine::primaryExpression()
{
    if (match(Token::OpenParen)) {
        expect(arithmeticExpression(), Compiler::Error::ExpectedExpr);
        expect(Token::CloseParen);
        return true;
    }
    
    std::string id;
    if (identifier(id)) {
        _exprStack.emplace_back(id);
        return true;
    }
        
    float f;
    if (floatValue(f)) {
        _exprStack.emplace_back(f);
        return true;
    }
        
    int32_t i;
    if (integerValue(i)) {
        _exprStack.emplace_back(i);
        return true;
    }
    return false;
}

bool
CloverCompileEngine::formalParameterList()
{
    Type t;
    if (match(Token::CloseParen)) {
        return true;
    }
    
    expect(type(t), Compiler::Error::ExpectedType);
    
    std::string id;
    expect(identifier(id), Compiler::Error::ExpectedIdentifier);
    
    _locals.emplace_back(id, _locals.size(), t, Symbol::Storage::Local);

    while (match(Token::Comma)) {
        expect(type(t), Compiler::Error::ExpectedType);
        expect(identifier(id), Compiler::Error::ExpectedIdentifier);
        _locals.emplace_back(id, _locals.size(), t, Symbol::Storage::Local);
    }
    
    // FIXME: Process these. Put them in a list for use by function?
    return true;
}

bool
CloverCompileEngine::argumentList()
{
    if (!arithmeticExpression()) {
        return true;
    }

    while (match(Token::Comma)) {
        expect(arithmeticExpression(), Compiler::Error::ExpectedExpr);
    }
    
    // FIXME: Process these. Put in registers starting at r0?
    return true;
}

bool
CloverCompileEngine::isReserved(Token token, const std::string str, Reserved& r)
{
    static std::map<std::string, Reserved> reserved = {
        { "struct",        Reserved::Struct },
    };

    if (CompileEngine::isReserved(token, str, r)) {
        return true;
    }

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

uint8_t
CloverCompileEngine::findInt(int32_t i)
{
    // Try to find an existing int const. If found, return
    // its address. If not found, create one and return 
    // that address.
    auto it = find_if(_rom32.begin(), _rom32.end(),
                    [i](uint32_t v) { return uint32_t(i) == v; });
    if (it != _rom32.end()) {
        return it - _rom32.begin();
    }
    
    _rom32.push_back(uint32_t(i));
    return _rom32.size() - 1;
}

uint8_t
CloverCompileEngine::findFloat(float f)
{
    // Try to find an existing fp const. If found, return
    // its address. If not found, create one and return 
    // that address.
    uint32_t i = floatToInt(f);
    auto it = find_if(_rom32.begin(), _rom32.end(),
                    [i](uint32_t v) { return i == v; });
    if (it != _rom32.end()) {
        return it - _rom32.begin();
    }
    
    _rom32.push_back(i);
    return _rom32.size() - 1;
}

CompileEngine::Symbol
CloverCompileEngine::findId(const std::string& s)
{
    auto it = find_if(_symbols.begin(), _symbols.end(),
                    [s](const Symbol& sym) { return sym._name == s; });

    if (it != _symbols.end()) {
        return *it;
    }
    
    // Not found. See if it's a local to the current function (param or var)
    it = find_if(_locals.begin(), _locals.end(),
                    [s](const Symbol& p) { return p._name == s; });

    expect(it != _locals.end(), Compiler::Error::UndefinedIdentifier);
    return *it;
}

uint8_t
CloverCompileEngine::addrFromId(const std::string& id)
{
    const Symbol& sym = findId(id);
    switch(sym._storage) {
        case CompileEngine::Symbol::Storage::Const:
            return sym._addr + ConstStart;
        case CompileEngine::Symbol::Storage::Global:
            return sym._addr + GlobalStart;
        case CompileEngine::Symbol::Storage::Local:
            return sym._addr + LocalStart;
    }
}

void
CloverCompileEngine::emitRHS()
{
    // Emit code for the top of the exprStack and leave the result in r0
    expect(!_exprStack.empty(), Compiler::Error::InternalError);
    const ExprEntry& entry = _exprStack.back();
    
    switch(entry.type()) {
        case ExprEntry::Type::None:
            _error = Compiler::Error::InternalError;
            throw true;
        case ExprEntry::Type::Id:
            // Emit Load id
            addOpRId(Op::Load, 0, addrFromId(entry));
            break;
        case ExprEntry::Type::Float:
            // Use an fp constant
            addOpRInt(Op::Load, 0, findFloat(entry));
            break;
        case ExprEntry::Type::Int: // int32_t
        {
            int32_t i = int32_t(entry);
            if (i >= -256 && i < 256) {
                bool neg = i < 0;
                if (neg) {
                    i = -i;
                }
                addOpRInt(Op::LoadIntConst, 0, i);
                if (neg) {
                    addOp(Op::NegInt);
                }
            } else {
                // Add an int const
                findInt(i);
                addOpRInt(Op::Load, 0, findInt(i));
            }
            break;
        }
    }
    
    _exprStack.pop_back();
}
