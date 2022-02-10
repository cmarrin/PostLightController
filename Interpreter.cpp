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
    execute(_initStart);
    if (_error == Error::None) {
        _error = _stack.error();
    }
    return _error == Error::None;
}

int32_t
Interpreter::loop()
{
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

        uint8_t id;
        uint8_t i;
        uint8_t index;
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
            case Op::PushRefX:
                id = getId();
                i = getI();
                value = _stack.pop();
                _stack.push(id + value * i);
                break;
            case Op::PushDeref:
                i = getI();
                value = _stack.pop();
                _stack.push(loadInt(value + i));
                break;
            case Op::PopDeref:
                i = getI();
                value = _stack.pop();
                index = _stack.pop() + i;
                storeInt(index, value);
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
                targ = (uint16_t(getId()) << 2) | (cmd & 0x03);
                _stack.push(_pc);
                _pc = targ + _codeOffset;
                
                if (Op(getUInt8ROM(_pc)) != Op::SetFrame) {
                    _error = Error::ExpectedSetFrame;
                    return -1;
                }
                break;
            case Op::CallNative:
                // FIXME: Probably better this be function pointers
                id = getConst();

                // Save the _pc just to make setFrame work
                _stack.push(_pc);

                switch(NativeFunction(id)) {
                    case NativeFunction::None:
                        _error = Error::InvalidNativeFunction;
                        return -1;
                    case NativeFunction::LoadColorParam: {
                        if (!_stack.setFrame(2, 0)) {
                            return -1;
                        }
                        uint32_t r = _stack.local(0);
                        uint32_t i = _stack.local(1);
                        _c[r] = Color(_params[i], _params[i + 1], _params[i + 2]);
                        _pc = _stack.restoreFrame(0);
                        break;
                    }
                    case NativeFunction::SetAllLights: {
                        if (!_stack.setFrame(1, 0)) {
                            return -1;
                        }
                        uint32_t r = _stack.local(0);
                        setAllLights(r);
                        _pc = _stack.restoreFrame(0);
                        break;
                    }
                    case NativeFunction::SetLight: {
                        if (!_stack.setFrame(2, 0)) {
                            return -1;
                        }
                        uint32_t i = _stack.local(0);
                        uint32_t r = _stack.local(1);
                        setLight(i, _c[r].rgb());
                        _pc = _stack.restoreFrame(0);
                        break;
                    }
                    case NativeFunction::Animate: {
                        if (!_stack.setFrame(1, 0)) {
                            return -1;
                        }
                        uint32_t i = _stack.local(0);
                        _pc = _stack.restoreFrame(animate(i));
                        
                        break;
                    }
                    case NativeFunction::Param: {
                        if (!_stack.setFrame(1, 0)) {
                            return -1;
                        }
                        uint32_t i = _stack.local(0);
                        _pc = _stack.restoreFrame(uint32_t(_params[i]));
                        break;
                    }
                    case NativeFunction::LoadColorComp: {
                        if (!_stack.setFrame(2, 0)) {
                            return -1;
                        }
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
                        _pc = _stack.restoreFrame(floatToInt(comp));
                        break;
                    }
                    case NativeFunction::StoreColorComp: {
                        if (!_stack.setFrame(3, 0)) {
                            return -1;
                        }
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
                        _pc = _stack.restoreFrame(0);
                        break;
                    }
                    case NativeFunction::Float: {
                        if (!_stack.setFrame(1, 0)) {
                            return -1;
                        }
                        uint32_t v = _stack.local(0);
                        _pc = _stack.restoreFrame(floatToInt(float(v)));
                        break;
                    }
                    case NativeFunction::Int: {
                        if (!_stack.setFrame(1, 0)) {
                            return -1;
                        }
                        float v = intToFloat(_stack.local(0));
                        _pc = _stack.restoreFrame(uint32_t(int32_t(v)));
                        break;
                    }
                }
                break;
            case Op::Return: {
                if (_stack.empty()) {
                    // Returning from top level is like Exit
                    return 0;
                }
                
                // TOS has return value. Pop it and push it back after restore
                _pc = _stack.restoreFrame(_stack.pop());
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
