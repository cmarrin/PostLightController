//
//  Compiler.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Compiler.h"

#include "Scanner.h"
#include <map>

using namespace arly;

class CompileEngine
{
public:
    enum class Error {
        None,
        ExpectedToken,
        ExpectedType,
        ExpectedValue,
        ExpectedOpcode,
    };
    
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
    
private:
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
        if (!match(Token::Identifier, "const")) {
            return false;
        }
        expect(type(), Error::ExpectedType);
        expect(Token::Identifier);
        expect(value(), Error::ExpectedValue);
        return true;
    }
    
    bool table()
    {
        if (!match(Token::Identifier, "table")) {
            return false;
        }
        expect(type(), Error::ExpectedType);
        expect(Token::Identifier);
        expect(Token::NewLine);
        tableEntries();
        expect(Token::Identifier, "end");
        return true;
    }
    
    bool effect()
    {
        if (!match(Token::Identifier, "effect")) {
            return false;
        }
        expect(Token::Identifier);
        expect(Token::Integer);
        expect(Token::NewLine);
        defs();
        init();
        loop();
        expect(Token::Identifier, "end");
        return true;
    }
    
    bool type()
    {
        if (match(Token::Identifier, "float")) {
            return true;
        }
        if (match(Token::Identifier, "uint8")) {
            return true;
        }
        if (match(Token::Identifier, "int32")) {
            return true;
        }
        return false;
    }
    
    bool values()
    {
        bool haveValues = false;
        while(1) {
            if (!value()) {
                break;
            }
            haveValues = true;
        }
        return haveValues;
    }

    bool value()
    {
        if (match(Token::Float)) {
            return true;
        }
        if (match(Token::Integer)) {
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
        if (!type()) {
            return false;
        }
        
        expect(Token::Identifier);
        expect(Token::Integer);
        return true;
    }

    bool init()
    {
        if (!match(Token::Identifier, "init")) {
            return false;
        }
        expect(Token::NewLine);
        statements();
        expect(Token::Identifier, "end");
        return true;
    }

    bool loop()
    {
        if (!match(Token::Identifier, "loop")) {
            return false;
        }
        expect(Token::NewLine);
        statements();
        expect(Token::Identifier, "end");
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
        static std::map<std::string, Op> opcodes = {
            { "LoadColorX    ", Op::LoadColorX     },
            { "LoadX         ", Op::LoadX          },
            { "StoreColorX   ", Op::StoreColorX    },
            { "StoreX        ", Op::StoreX         },
            { "MoveColor     ", Op::MoveColor      },
            { "Move          ", Op::Move           },
            { "LoadVal       ", Op::LoadVal        },
            { "StoreVal      ", Op::StoreVal       },
            { "MinInt        ", Op::MinInt         },
            { "MinFloat      ", Op::MinFloat       },
            { "MaxInt        ", Op::MaxInt         },
            { "MaxFloat      ", Op::MaxFloat       },
            { "SetLight      ", Op::SetLight       },
            { "Init          ", Op::Init           },
            { "Random        ", Op::Random         },
            { "If            ", Op::If             },
            { "Else          ", Op::Else           },
            { "Bor           ", Op::Bor            },
            { "Bxor          ", Op::Bxor           },
            { "Band          ", Op::Band           },
            { "Bnot          ", Op::Bnot           },
            { "Or            ", Op::Or             },
            { "And           ", Op::And            },
            { "Not           ", Op::Not            },
            { "LTInt         ", Op::LTInt          },
            { "LTFloat       ", Op::LTFloat        },
            { "LEInt         ", Op::LEInt          },
            { "LEFloat       ", Op::LEFloat        },
            { "EQInt         ", Op::EQInt          },
            { "EQFloat       ", Op::EQFloat        },
            { "NEInt         ", Op::NEInt          },
            { "NEFloat       ", Op::NEFloat        },
            { "GEInt         ", Op::GEInt          },
            { "GEFloat       ", Op::GEFloat        },
            { "GTInt         ", Op::GTInt          },
            { "GTFloat       ", Op::GTFloat        },
            { "AddInt        ", Op::AddInt         },
            { "AddFloat      ", Op::AddFloat       },
            { "SubInt        ", Op::SubInt         },
            { "SubFloat      ", Op::SubFloat       },
            { "MulInt        ", Op::MulInt         },
            { "MulFloat      ", Op::MulFloat       },
            { "DivInt        ", Op::DivInt         },
            { "DivFloat      ", Op::DivFloat       },
            { "NegInt        ", Op::NegInt         },
            { "NegFloat      ", Op::NegFloat       },
            { "LoadColorParam", Op::LoadColorParam },
            { "LoadIntParam  ", Op::LoadIntParam   },
            { "LoadFloatParam", Op::LoadFloatParam },
            { "LoadColor     ", Op::LoadColor      },
            { "Load          ", Op::Load           },
            { "StoreColor    ", Op::StoreColor     },
            { "Store         ", Op::Store          },
            { "LoadBlack     ", Op::LoadBlack      },
            { "LoadZero      ", Op::LoadZero       },
            { "LoadIntOne    ", Op::LoadIntOne     },
            { "LoadFloatOne  ", Op::LoadFloatOne   },
            { "LoadByteMax   ", Op::LoadByteMax    },
            { "Return        ", Op::Return         },
            { "ToFloat       ", Op::ToFloat        },
            { "ToInt         ", Op::ToInt          },
            { "SetAllLights  ", Op::SetAllLights   },
            { "ForEach       ", Op::ForEach        },
        };
        
        if (_scanner.getToken() != Token::Identifier) {
            return false;
        }
        
        auto it = opcodes.find(_scanner.getTokenString());
        expect(it != opcodes.end(), Error::ExpectedOpcode);
        values();
        return true;
    }
    
    bool forStatement()
    {
        if (!match(Token::Identifier, "foreach")) {
            return false;
        }
        expect(Token::Identifier);
        expect(Token::NewLine);
        statements();
        expect(Token::Identifier, "end");
        return true;
    }
    
    bool ifStatement()
    {
        if (!match(Token::Identifier, "if")) {
            return false;
        }
        expect(Token::NewLine);
        statements();
        
        if (match(Token::Identifier, "else")) {
            expect(Token::NewLine);
            statements();
        }
        
        expect(Token::Identifier, "end");
        return true;
    }
    
    void expect(bool passed, Error error)
    {
        if (!passed) {
            _error = error;
            throw true;
        }
    }
    
    void expect(Token token, const char* str = nullptr)
    {
        bool err = false;
        if (_scanner.getToken() != token) {
            _error = Error::ExpectedToken;
            err = true;
        }
        
        if (str && _scanner.getTokenString() != str) {
            _error = Error::ExpectedToken;
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
    
    Scanner _scanner;
    Error _error = Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
};

bool Compiler::compile(std::istream* stream)
{
    CompileEngine eng(stream);
    
    eng.program();
    
    return false;
}

