//
//  Compiler.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Interpreter.h"

#include "Opcodes.h"

using namespace arly;

bool
Interpreter::init(uint8_t cmd, const uint8_t* buf, uint8_t size)
{
    memcpy(_params, buf, size);
    _paramsSize = size;
	_error = Error::None;
 
    if (_global) {
        delete [ ]_global;
        _global = nullptr;
        _globalSize = 0;
    }
    
    _constOffset = 8;
    
    uint32_t constSize = uint32_t(getUInt8ROM(4)) * 4;
    bool found = false;
    _codeOffset = _constOffset + constSize;
    
    // Alloc globals
    _globalSize = getUInt8ROM(5);
    
    if (_globalSize) {
        _global = new uint32_t[_globalSize];
    }
    
    // Alloc stack
    _stack.alloc(getUInt8ROM(6));

    // Find command
    while (1) {
        uint8_t c = rom(_codeOffset);
        if (c == 0) {
            _codeOffset++;
            break;
        }
        if (c == cmd) {
            // found cmd
            _numParams = getUInt8ROM(_codeOffset + 1);
            _initStart = getUInt16ROM(_codeOffset + 2);
            _loopStart = getUInt16ROM(_codeOffset + 4);
            
            found = true;
            
            //Need to keep looping through all effects
            // to find where the code starts
        }
        
        _codeOffset += 6;
    }
    
    if (!found) {
        _error = Error::CmdNotFound;
        _errorAddr = -1;
        return false;
    }

    _initStart += _codeOffset;
    _loopStart += _codeOffset;
    
    // Execute init();
    if (Op(getUInt8ROM(_initStart)) != Op::SetFrame) {
        _error = Error::ExpectedSetFrame;
        return false;
    }

    // Push a dummy pc, for the return
    _stack.push(uint32_t(-1));
    execute(_initStart);
    if (_error == Error::None) {
        _error = _stack.error();
    }
    return _error == Error::None;
}

int32_t
Interpreter::loop()
{
    if (Op(getUInt8ROM(_loopStart)) != Op::SetFrame) {
        _error = Error::ExpectedSetFrame;
        return false;
    }

    // Push a dummy pc, for the return
    _stack.push(uint32_t(-1));
    return execute(_loopStart);
}

int32_t
Interpreter::execute(uint16_t addr)
{
    _pc = addr;
    
    while(1) {
        if (_stack.error() != Error::None) {
            _error = _stack.error();
        }
        if (_error != Error::None) {
            _errorAddr = _pc - 1;
            return -1;
        }
        
        uint8_t cmd = getUInt8ROM(_pc++);
        uint8_t index = 0;
        if (cmd >= 0x80) {
            index = cmd & 0x0f;
            cmd &= 0xf0;
        }

        uint8_t id;
        uint8_t i;
        uint16_t targ;
        uint16_t addr;
        uint8_t numParams;
        uint8_t numLocals;
        uint32_t value;
        
        switch(Op(cmd)) {
			default:
				_error = Error::InvalidOp;
				return -1;
            case Op::Push:
                id = getId();
                _stack.push(loadInt(id));
                break;
            case Op::Pop:
                id = getId();
                storeInt(id, _stack.pop());
                break;
            case Op::PushZero:
                _stack.push(0);
                break;
            case Op::PushIntConst:
                _stack.push(getConst());
                break;
                
            case Op::PushRef:
                _stack.push(getId());
                break;
            case Op::PushDeref:
                value = _stack.pop();
                _stack.push(loadInt(value));
                break;
            case Op::PopDeref:
                value = _stack.pop();
                index = _stack.pop();
                storeInt(index, value);
                break;

            case Op::Offset:
                _stack.top() += index;
                break;
            case Op::Index:
                value = _stack.pop();
                _stack.top() += value * index;
                break;

            case Op::Dup:
                _stack.push(_stack.top());
                break;
            case Op::Drop:
                _stack.pop();
                break;
            case Op::Swap:
                _stack.swap();
                break;

            case Op::MinInt:
                _stack.push(min(int32_t(_stack.pop()), int32_t(_stack.pop())));
                break;
            case Op::MinFloat:
                _stack.push(floatToInt(min(intToFloat(_stack.pop()), intToFloat(_stack.pop()))));
                break;
            case Op::MaxInt:
                _stack.push(max(int32_t(_stack.pop()), int32_t(_stack.pop())));
                break;
            case Op::MaxFloat:
                _stack.push(floatToInt(max(intToFloat(_stack.pop()), intToFloat(_stack.pop()))));
                break;

            case Op::Exit:
                return _stack.pop();
            case Op::ToFloat:
                _stack.top() = floatToInt(float(int32_t(_stack.top())));
                break;
            case Op::ToInt:
                _stack.top() = uint32_t(int32_t(intToFloat(_stack.top())));
                break;

            case Op::Init:
                id = getId();
                
                // Only global or local
                if (id < GlobalStart) {
                    _error = Error::OnlyMemAddressesAllowed;
                    return -1;
                }
                
                value = _stack.pop();
                if (id < LocalStart) {
                    addr = id - GlobalStart;
                    memset(_global + addr, _stack.pop(), value * sizeof(uint32_t));
                } else {
                    addr = id - LocalStart;
                    memset(&_stack.local(addr), _stack.pop(), value * sizeof(uint32_t));
                }
                break;
            case Op::RandomInt: {
                int32_t max = _stack.pop();
                int32_t min = _stack.pop();
                _stack.push(uint32_t(random(min, max)));
                break;
            }
            case Op::RandomFloat: {
                float max = intToFloat(_stack.pop());
                float min = intToFloat(_stack.pop());
                _stack.push(floatToInt(random(min, max)));
                break;
            }

            case Op::If:
                id = getSz();
                if (_stack.pop() == 0) {
                    // Skip if
                    _pc += id;
                    
                    // Next instruction must be EndIf or Else
                    cmd = getUInt8ROM(_pc++);
                    if (Op(cmd) == Op::EndIf) {
                        // We hit the end of the if, just continue
                    } else if (Op(cmd) == Op::Else) {
                        // We have an Else, execute it
                        getSz(); // Ignore Sz
                    } else {
                        _error = Error::UnexpectedOpInIf;
                        return -1;
                    }
                }
                break;
            case Op::Else:
                // If we get here the corresponding If succeeded so ignore this
                _pc += getSz();
                break;
            case Op::EndIf:
                // This is the end of an if, always ignore it
                break;

            case Op::ForEach:
                if (_foreachSz >= 0) {
                    // Can't do nested for loops
                    _error = Error::NestedForEachNotAllowed;
                    return -1;
                }
                
                _foreachId = getId();
                _foreachSz = getSz();
                _foreachCount = int32_t(_stack.pop());
                _foreachLoopAddr = _pc;
                
                // See if we've already gone past count
                if (int32_t(loadInt(_foreachId)) >= _foreachCount) {
                    _pc += _foreachSz + 1;
                    _foreachSz = -1;
                }
                break;
            case Op::EndForEach: {
                // End of loop. Iterate and check for end
                int32_t i = loadInt(_foreachId) + 1;
                storeInt(_foreachId, i);
                if (i < _foreachCount) {
                    _pc = _foreachLoopAddr;
                } else {
                    // Finished loop
                    _foreachSz = -1;
                }
                break;
            }
            
            case Op::Call:
                targ = uint16_t(getId()) | (uint16_t(index)  << 8);
                _stack.push(_pc);
                _pc = targ + _codeOffset;
                
                if (Op(getUInt8ROM(_pc)) != Op::SetFrame) {
                    _error = Error::ExpectedSetFrame;
                    return -1;
                }
                break;
            case Op::CallNative: {
                // FIXME: Probably better this be function pointers
                id = getConst();

                // Save the _pc just to make setFrame work
                _stack.push(_pc);
                
                uint8_t numParams = 0;
                
                switch(NativeFunction(id)) {
                    case NativeFunction::None:
                        _error = Error::InvalidNativeFunction;
                        return -1;
                    case NativeFunction::LoadColorParam: numParams = 2; break;
                    case NativeFunction::SetAllLights: numParams = 1; break;
                    case NativeFunction::SetLight: numParams = 2; break;
                    case NativeFunction::Animate: numParams = 1; break;
                    case NativeFunction::Param: numParams = 1; break;
                    case NativeFunction::LoadColorComp: numParams = 2; break;
                    case NativeFunction::StoreColorComp: numParams = 3; break;
                    case NativeFunction::Float: numParams = 1; break;
                    case NativeFunction::Int: numParams = 1; break;
                    case NativeFunction::LogInt: numParams = 1; break;
                    case NativeFunction::LogFloat: numParams = 1; break;
                    case NativeFunction::LogColor: numParams = 1; break;
                    case NativeFunction::RandomInt: numParams = 2; break;
                    case NativeFunction::RandomFloat: numParams = 2; break;
                    case NativeFunction::InitArray: numParams = 3; break;
                }

                if (!_stack.setFrame(numParams, 0)) {
                    return -1;
                }
                
                uint32_t returnVal = 0;

                switch(NativeFunction(id)) {
                    case NativeFunction::None:
                        _error = Error::InvalidNativeFunction;
                        return -1;
                    case NativeFunction::LoadColorParam: {
                        uint32_t r = _stack.local(0);
                        uint32_t i = _stack.local(1);
                        _c[r] = Color(_params[i], _params[i + 1], _params[i + 2]);
                        break;
                    }
                    case NativeFunction::SetAllLights: {
                        uint32_t r = _stack.local(0);
                        setAllLights(r);
                        break;
                    }
                    case NativeFunction::SetLight: {
                        uint32_t i = _stack.local(0);
                        uint32_t r = _stack.local(1);
                        setLight(i, _c[r].rgb());
                        break;
                    }
                    case NativeFunction::Animate: {
                        uint32_t i = _stack.local(0);
                        returnVal = animate(i);
                        break;
                    }
                    case NativeFunction::Param: {
                        uint32_t i = _stack.local(0);
                        returnVal = uint32_t(_params[i]);
                        break;
                    }
                    case NativeFunction::LoadColorComp: {
                        uint32_t c = _stack.local(0);
                        uint32_t i = _stack.local(1);
                        float comp;
                        switch(i) {
                            case 0: comp = _c[c].hue(); break;
                            case 1: comp = _c[c].sat(); break;
                            case 2: comp = _c[c].val(); break;
                            default:
                                _error = Error::InvalidColorComp;
                                return -1;
                        }
                        returnVal = floatToInt(comp);
                        break;
                    }
                    case NativeFunction::StoreColorComp: {
                        uint32_t c = _stack.local(0);
                        uint32_t i = _stack.local(1);
                        float v = intToFloat(_stack.local(2));
                        switch(i) {
                            case 0: _c[c].setHue(v); break;
                            case 1: _c[c].setSat(v); break;
                            case 2: _c[c].setVal(v); break;
                            default:
                                _error = Error::InvalidColorComp;
                                return -1;
                        }
                        break;
                    }
                    case NativeFunction::Float: {
                        uint32_t v = _stack.local(0);
                        returnVal = floatToInt(float(v));
                        break;
                    }
                    case NativeFunction::Int: {
                        float v = intToFloat(_stack.local(0));
                        returnVal = uint32_t(int32_t(v));
                        break;
                    }
                    case NativeFunction::LogInt: {
                        log(_pc - 1, 0, int32_t(_stack.local(0)));
                        break;
                    }
                    case NativeFunction::LogFloat: {
                        logFloat(_pc - 1, 0, intToFloat(_stack.local(0)));
                        break;
                    }
                    case NativeFunction::LogColor: {
                        uint32_t i = _stack.local(0);
                        logColor(_pc - 1, i, _c[i]);
                        break;
                    }
                    case NativeFunction::RandomInt: {
                        int32_t min = _stack.local(0);
                        int32_t max = _stack.local(1);
                        returnVal = uint32_t(random(min, max));
                        break;
                    }
                    case NativeFunction::RandomFloat: {
                        float min = intToFloat(_stack.local(0));
                        float max = intToFloat(_stack.local(1));
                        returnVal = floatToInt(random(min, max));
                        break;
                    }
                    case NativeFunction::InitArray: {
                        uint32_t i = _stack.local(0);
                        uint32_t v = _stack.local(1);
                        uint32_t n = _stack.local(2);

                        // Only global or local
                        if (i < GlobalStart) {
                            _error = Error::OnlyMemAddressesAllowed;
                            return -1;
                        }
                
                        if (i < LocalStart) {
                            addr = i - GlobalStart;
                            memset(_global + addr, v, n * sizeof(uint32_t));
                        } else {
                            addr = i - LocalStart;
                            memset(&_stack.local(addr), v, n * sizeof(uint32_t));
                        }
                        break;
                    }
                }
                
                _pc = _stack.restoreFrame(returnVal);
                break;
            }
            case Op::Return: {
                uint32_t retVal = _stack.empty() ? 0 : _stack.pop();
                
                if (_stack.empty()) {
                    // Returning from top level is like Exit
                    return 0;
                }
                
                // TOS has return value. Pop it and push it back after restore
                _pc = _stack.restoreFrame(retVal);
                
                // A _pc of -1 is like an exit
                if (_pc < 0) {
                    // retVal was pushed, get rid of it
                    _stack.pop();
                    return retVal;
                }
                break;
            }
            case Op::SetFrame:
                getPL(numParams, numLocals);
                if (!_stack.setFrame(numParams, numLocals)) {
                    return -1;
                }
                break;

            case Op::Or:    _stack.top() |= _stack.pop(); break;
            case Op::Xor:   _stack.top() ^= _stack.pop(); break;
            case Op::And:   _stack.top() &= _stack.pop(); break;
            case Op::Not:   _stack.top() = ~_stack.top(); break;
            case Op::LOr:   _stack.push(_stack.pop() || _stack.pop()); break;
            case Op::LAnd:   _stack.push(_stack.pop() && _stack.pop()); break;
            case Op::LNot:   _stack.top() = !_stack.top(); break;

            case Op::LTInt:
                value = _stack.pop();
                _stack.top() = int32_t(_stack.top()) < int32_t(value);
                break;
            case Op::LTFloat:
                value = _stack.pop();
                _stack.top() = intToFloat(_stack.top()) < intToFloat(value);
                break;
            case Op::LEInt:
                value = _stack.pop();
                _stack.top() = int32_t(_stack.top()) <= int32_t(value);
                break;
            case Op::LEFloat:
                value = _stack.pop();
                _stack.top() = intToFloat(_stack.top()) <= intToFloat(value);
                break;
            case Op::EQInt:
                value = _stack.pop();
                _stack.top() = int32_t(_stack.top()) == int32_t(value);
                break;
            case Op::EQFloat:
                value = _stack.pop();
                _stack.top() = intToFloat(_stack.top()) == intToFloat(value);
                break;
            case Op::NEInt:
                value = _stack.pop();
                _stack.top() = int32_t(_stack.top()) != int32_t(value);
                break;
            case Op::NEFloat:
                value = _stack.pop();
                _stack.top() = intToFloat(_stack.top()) != intToFloat(value);
                break;
            case Op::GEInt:
                value = _stack.pop();
                _stack.top() = int32_t(_stack.top()) >= int32_t(value);
                break;
            case Op::GEFloat:
                value = _stack.pop();
                _stack.top() = intToFloat(_stack.top()) >= intToFloat(value);
                break;
            case Op::GTInt:
                value = _stack.pop();
                _stack.top() = int32_t(_stack.top()) > int32_t(value);
                break;
            case Op::GTFloat:
                value = _stack.pop();
                _stack.top() = intToFloat(_stack.top()) > intToFloat(value);
                break;

            case Op::AddInt:
                value = _stack.pop();
                _stack.top() = int32_t(_stack.top()) + int32_t(value);
                break;
            case Op::AddFloat:
                value = _stack.pop();
                _stack.top() = floatToInt(intToFloat(_stack.top()) + intToFloat(value));
                break;
            case Op::SubInt:
                value = _stack.pop();
                _stack.top() = int32_t(_stack.top()) - int32_t(value);
                break;
            case Op::SubFloat:
                value = _stack.pop();
                _stack.top() = floatToInt(intToFloat(_stack.top()) - intToFloat(value));
                break;
            case Op::MulInt:
                value = _stack.pop();
                _stack.top() = int32_t(_stack.top()) * int32_t(value);
                break;
            case Op::MulFloat:
                value = _stack.pop();
                _stack.top() = floatToInt(intToFloat(_stack.top()) * intToFloat(value));
                break;
            case Op::DivInt:
                value = _stack.pop();
                _stack.top() = int32_t(_stack.top()) / int32_t(value);
                break;
            case Op::DivFloat:
                value = _stack.pop();
                _stack.top() = floatToInt(intToFloat(_stack.top()) / intToFloat(value));
                break;
            case Op::NegInt:
                _stack.top() = -int32_t(_stack.top());
                break;
            case Op::NegFloat:
                _stack.top() = floatToInt(-intToFloat(_stack.top()));
                break;

            case Op::Log:
                // Log tos-i and current addr
                i = getI();
                log(_pc - 1, i, int32_t(_stack.top(i)));
                break;
            case Op::LogFloat:
                // Log tos-i and current addr
                i = getI();
                logFloat(_pc - 1, i, intToFloat(_stack.top(i)));
                break;
            case Op::LogColor:
                // Log color[i] and current addr
                i = getI();
                logColor(_pc - 1, i, _c[i]);
                break;
        }
    }
}

void
Interpreter::setAllLights(uint8_t r)
{
    uint32_t rgb = _c[r].rgb();
    uint8_t n = numPixels();
    for (uint8_t i = 0; i < n; ++i) {
        setLight(i, rgb);
    }
}

int32_t
Interpreter::animate(uint32_t index)
{
    float cur = loadFloat(index, 0);
    float inc = loadFloat(index, 1);
    float min = loadFloat(index, 2);
    float max = loadFloat(index, 3);

    cur += inc;
    storeFloat(index, 0, cur);

    if (0 < inc) {
        if (cur >= max) {
            cur = max;
            inc = -inc;
            storeFloat(index, 0, cur);
            storeFloat(index, 1, inc);
        }
    } else {
        if (cur <= min) {
            cur = min;
            inc = -inc;
            storeFloat(index, 0, cur);
            storeFloat(index, 1, inc);
            return 1;
        }
    }
    return 0;
}
