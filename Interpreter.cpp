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
    _stackk.alloc(getUInt8ROM(6));

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
    try {
        execute(_initStart);
    }
    catch(...) {
        return false;
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
        if (_error != Error::None) {
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
                _errorAddr = _pc - 1;
				return -1;
            case Op::Push:
                id = getId();
                _stackk.push(loadInt(id));
                break;
            case Op::Pop:
                id = getId();
                storeInt(id, _stackk.pop());
                break;
            case Op::PushZero:
                _stackk.push(0);
                break;
            case Op::PushIntConst:
                _stackk.push(getConst());
                break;
                
            case Op::PushRef:
                _stackk.push(getId());
                break;
            case Op::PushRefX:
                id = getId();
                i = getI();
                value = _stackk.pop();
                _stackk.push(id + value * i);
                break;
            case Op::PushDeref:
                i = getI();
                value = _stackk.pop();
                _stackk.push(loadInt(value + i));
                break;
            case Op::PopDeref:
                i = getI();
                value = _stackk.pop();
                index = value + i;
                storeInt(index, _stackk.pop());
                break;

            case Op::Dup:
                _stackk.push(_stackk.top());
                break;
            case Op::Drop:
                _stackk.pop();
                break;
            case Op::Swap:
                _stackk.swap();
                break;

            case Op::MinInt:
                _stackk.push(min(int32_t(_stackk.pop()), int32_t(_stackk.pop())));
                break;
            case Op::MinFloat:
                _stackk.push(floatToInt(min(intToFloat(_stackk.pop()), intToFloat(_stackk.pop()))));
                break;
            case Op::MaxInt:
                _stackk.push(max(int32_t(_stackk.pop()), int32_t(_stackk.pop())));
                break;
            case Op::MaxFloat:
                _stackk.push(floatToInt(max(intToFloat(_stackk.pop()), intToFloat(_stackk.pop()))));
                break;

            case Op::Exit:
                return _stackk.pop();
            case Op::ToFloat:
                _stackk.top() = floatToInt(float(int32_t(_stackk.top())));
                break;
            case Op::ToInt:
                _stackk.top() = uint32_t(int32_t(intToFloat(_stackk.top())));
                break;

            case Op::Init:
                id = getId();
                
                // Only global or local
                if (id < GlobalStart) {
                    _error = Error::OnlyMemAddressesAllowed;
                    _errorAddr = _pc - 1;
                    return -1;
                }
                
                value = _stackk.pop();
                if (id < LocalStart) {
                    addr = id - GlobalStart;
                    memset(_global + addr, _stackk.pop(), value * sizeof(uint32_t));
                } else {
                    addr = id - LocalStart;
                    memset(&_stackk.local(addr), _stackk.pop(), value * sizeof(uint32_t));
                }
                break;
            case Op::RandomInt: {
                int32_t max = _stackk.pop();
                int32_t min = _stackk.pop();
                _stackk.push(uint32_t(random(min, max)));
                break;
            }
            case Op::RandomFloat: {
                float max = intToFloat(_stackk.pop());
                float min = intToFloat(_stackk.pop());
                _stackk.push(floatToInt(random(min, max)));
                break;
            }

            case Op::If:
                id = getSz();
                if (_stackk.pop() == 0) {
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
                        _errorAddr = _pc - 1;
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
                    _errorAddr = _pc - 1;
                    return -1;
                }
                
                _foreachId = getId();
                _foreachSz = getSz();
                _foreachCount = _stackk.pop();
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
                _stackk.push(_pc);
                _pc = targ + _codeOffset;
                
                if (Op(getUInt8ROM(_pc)) != Op::SetFrame) {
                    _error = Error::ExpectedSetFrame;
                    _errorAddr = _pc - 1;
                    return -1;
                }
                break;
            case Op::CallNative:
                // FIXME: Probably better this be function pointers
                id = getConst();

                // Save the _pc just to make setFrame work
                _stackk.push(_pc);

                switch(NativeFunction(id)) {
                    case NativeFunction::None:
                        _error = Error::InvalidNativeFunction;
                        _errorAddr = _pc - 1;
                        return -1;
                    case NativeFunction::LoadColorParam: {
                        _stackk.setFrame(2, 0);
                        uint32_t r = _stackk.local(0);
                        uint32_t i = _stackk.local(1);
                        _c[r] = Color(_params[i], _params[i + 1], _params[i + 2]);
                        _pc = _stackk.restoreFrame(0);
                        break;
                    }
                    case NativeFunction::SetAllLights: {
                        _stackk.setFrame(1, 0);
                        uint32_t r = _stackk.local(0);
                        setAllLights(r);
                        _pc = _stackk.restoreFrame(0);
                        break;
                    }
                    case NativeFunction::SetLight: {
                        _stackk.setFrame(2, 0);
                        uint32_t i = _stackk.local(0);
                        uint32_t r = _stackk.local(1);
                        setLight(i, _c[r].rgb());
                        _pc = _stackk.restoreFrame(0);
                        break;
                    }
                    case NativeFunction::Animate: {
                        _stackk.setFrame(1, 0);
                        uint32_t i = _stackk.local(0);
                        _pc = _stackk.restoreFrame(animate(i));
                        
                        break;
                    }
                    case NativeFunction::Param: {
                        _stackk.setFrame(1, 0);
                        uint32_t i = _stackk.local(0);
                        _pc = _stackk.restoreFrame(uint32_t(_params[i]));
                        break;
                    }
                    case NativeFunction::LoadColorComp: {
                        _stackk.setFrame(2, 0);
                        uint32_t c = _stackk.local(0);
                        uint32_t i = _stackk.local(1);
                        float comp;
                        switch(i) {
                            case 0: comp = _c[c].hue(); break;
                            case 1: comp = _c[c].sat(); break;
                            case 2: comp = _c[c].val(); break;
                            default:
                                _error = Error::InvalidColorComp;
                                _errorAddr = _pc - 1;
                                return -1;
                        }
                        _pc = _stackk.restoreFrame(floatToInt(comp));
                        break;
                    }
                    case NativeFunction::StoreColorComp: {
                        _stackk.setFrame(3, 0);
                        uint32_t c = _stackk.local(0);
                        uint32_t i = _stackk.local(1);
                        float v = intToFloat(_stackk.local(2));
                        switch(i) {
                            case 0: _c[c].setHue(v); break;
                            case 1: _c[c].setSat(v); break;
                            case 2: _c[c].setVal(v); break;
                            default:
                                _error = Error::InvalidColorComp;
                                _errorAddr = _pc - 1;
                                return -1;
                        }
                        _pc = _stackk.restoreFrame(0);
                        break;
                    }
                    case NativeFunction::Float: {
                        _stackk.setFrame(1, 0);
                        uint32_t v = _stackk.local(0);
                        _pc = _stackk.restoreFrame(floatToInt(float(v)));
                        break;
                    }
                    case NativeFunction::Int: {
                        _stackk.setFrame(1, 0);
                        float v = intToFloat(_stackk.local(0));
                        _pc = _stackk.restoreFrame(uint32_t(int32_t(v)));
                        break;
                    }
                }
                break;
            case Op::Return: {
                if (_stackk.empty()) {
                    // Returning from top level is like Exit
                    return 0;
                }
                
                // TOS has return value. Pop it and push it back after restore
                _pc = _stackk.restoreFrame(_stackk.pop());
                break;
            }
            case Op::SetFrame:
                getPL(numParams, numLocals);
                _stackk.setFrame(numParams, numLocals);
                break;

            case Op::Or:    _stackk.top() |= _stackk.pop(); break;
            case Op::Xor:   _stackk.top() ^= _stackk.pop(); break;
            case Op::And:   _stackk.top() &= _stackk.pop(); break;
            case Op::Not:   _stackk.top() = ~_stackk.top(); break;
            case Op::LOr:   _stackk.push(_stackk.pop() || _stackk.pop()); break;
            case Op::LAnd:   _stackk.push(_stackk.pop() && _stackk.pop()); break;
            case Op::LNot:   _stackk.top() = !_stackk.top(); break;

            case Op::LTInt:
                value = _stackk.pop();
                _stackk.top() = int32_t(_stackk.top()) < int32_t(value);
                break;
            case Op::LTFloat:
                value = _stackk.pop();
                _stackk.top() = intToFloat(_stackk.top()) < intToFloat(value);
                break;
            case Op::LEInt:
                value = _stackk.pop();
                _stackk.top() = int32_t(_stackk.top()) <= int32_t(value);
                break;
            case Op::LEFloat:
                value = _stackk.pop();
                _stackk.top() = intToFloat(_stackk.top()) <= intToFloat(value);
                break;
            case Op::EQInt:
                value = _stackk.pop();
                _stackk.top() = int32_t(_stackk.top()) == int32_t(value);
                break;
            case Op::EQFloat:
                value = _stackk.pop();
                _stackk.top() = intToFloat(_stackk.top()) == intToFloat(value);
                break;
            case Op::NEInt:
                value = _stackk.pop();
                _stackk.top() = int32_t(_stackk.top()) != int32_t(value);
                break;
            case Op::NEFloat:
                value = _stackk.pop();
                _stackk.top() = intToFloat(_stackk.top()) != intToFloat(value);
                break;
            case Op::GEInt:
                value = _stackk.pop();
                _stackk.top() = int32_t(_stackk.top()) >= int32_t(value);
                break;
            case Op::GEFloat:
                value = _stackk.pop();
                _stackk.top() = intToFloat(_stackk.top()) >= intToFloat(value);
                break;
            case Op::GTInt:
                value = _stackk.pop();
                _stackk.top() = int32_t(_stackk.top()) > int32_t(value);
                break;
            case Op::GTFloat:
                value = _stackk.pop();
                _stackk.top() = intToFloat(_stackk.top()) > intToFloat(value);
                break;

            case Op::AddInt:
                value = _stackk.pop();
                _stackk.top() = int32_t(_stackk.top()) + int32_t(value);
                break;
            case Op::AddFloat:
                value = _stackk.pop();
                _stackk.top() = floatToInt(intToFloat(_stackk.top()) + intToFloat(value));
                break;
            case Op::SubInt:
                value = _stackk.pop();
                _stackk.top() = int32_t(_stackk.top()) - int32_t(value);
                break;
            case Op::SubFloat:
                value = _stackk.pop();
                _stackk.top() = floatToInt(intToFloat(_stackk.top()) - intToFloat(value));
                break;
            case Op::MulInt:
                value = _stackk.pop();
                _stackk.top() = int32_t(_stackk.top()) * int32_t(value);
                break;
            case Op::MulFloat:
                value = _stackk.pop();
                _stackk.top() = floatToInt(intToFloat(_stackk.top()) * intToFloat(value));
                break;
            case Op::DivInt:
                value = _stackk.pop();
                _stackk.top() = int32_t(_stackk.top()) / int32_t(value);
                break;
            case Op::DivFloat:
                value = _stackk.pop();
                _stackk.top() = floatToInt(intToFloat(_stackk.top()) / intToFloat(value));
                break;
            case Op::NegInt:
                _stackk.top() = -int32_t(_stackk.top());
                break;
            case Op::NegFloat:
                _stackk.top() = floatToInt(-intToFloat(_stackk.top()));
                break;

            case Op::Log:
                // Log tos-i and current addr
                i = getI();
                log(_pc - 1, i, int32_t(_stackk.top(i)));
                break;
            case Op::LogFloat:
                // Log tos-i and current addr
                i = getI();
                logFloat(_pc - 1, i, intToFloat(_stackk.top(i)));
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
