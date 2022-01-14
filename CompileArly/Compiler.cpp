//
//  Compiler.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Compiler.h"

#include "CompileEngine.h"

#include <map>
#include <vector>

using namespace arly;

bool Compiler::compile(std::istream* istream, std::vector<uint8_t>& executable)
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
