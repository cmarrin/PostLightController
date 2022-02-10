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
 
    if (_stack) {
        delete [ ]_stack;
        _stack = nullptr;
        _stackSize = 0;
    }
    
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
    _stackSize = getUInt8ROM(6);
    
    if (_stackSize) {
        _stack = new uint32_t[_stackSize];
    }
    
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
        uint8_t t;
        uint8_t i;
        uint8_t index;
        uint16_t targ;
        uint16_t addr;
        uint8_t numParams;
        uint8_t numLocals;
        uint32_t rhs;
        
        switch(Op(cmd)) {
			default:
				_error = Error::InvalidOp;
                _errorAddr = _pc - 1;
				return -1;
            case Op::Push:
                id = getId();
                _stack[_sp++] = loadInt(id);
                break;
            case Op::Pop:
                id = getId();
                storeInt(id, _stack[--_sp]);
                break;
            case Op::PushZero:
                _stack[_sp++] = 0;
                break;
            case Op::PushIntConst:
                _stack[_sp++] = getConst();
                break;
                
            case Op::PushRef:
                _stack[_sp++] = getId();
                break;
            case Op::PushRefX:
                id = getId();
                i = getI();
                t = _stack[--_sp];
                _stack[_sp++] = id + t * i;
                break;
            case Op::PushDeref:
                i = getI();
                t = _stack[--_sp];
                _stack[_sp++] = loadInt(t + i);
                break;
            case Op::PopDeref:
                i = getI();
                t = _stack[--_sp];
                index = t + i;
                storeInt(index, _stack[--_sp]);
                break;

            case Op::Dup:
                _stack[_sp++] = _stack[_sp-1];
                break;
            case Op::Drop:
                _sp--;
                break;
            case Op::Swap:
                rhs = _stack[_sp-1];
                _stack[_sp-1] = _stack[_sp-2];
                _stack[_sp-2] = rhs;
                break;

            case Op::MinInt:
                rhs = _stack[--_sp];
                _stack[_sp++] = min(int32_t(rhs), int32_t(_stack[--_sp]));
                break;
            case Op::MinFloat:
                rhs = _stack[--_sp];
                _stack[_sp++] = min(intToFloat(rhs), intToFloat(_stack[--_sp]));
                break;
            case Op::MaxInt:
                rhs = _stack[--_sp];
                _stack[_sp++] = max(int32_t(rhs), int32_t(_stack[--_sp]));
                break;
            case Op::MaxFloat:
                rhs = _stack[--_sp];
                _stack[_sp++] = max(intToFloat(rhs), intToFloat(_stack[--_sp]));
                break;

            case Op::Exit:
                return _stack[--_sp];
            case Op::ToFloat:
                _stack[_sp] = floatToInt(float(int32_t(_stack[_sp])));
                break;
            case Op::ToInt:
                _stack[_sp] = uint32_t(int32_t(intToFloat(_stack[_sp])));
                break;

            case Op::Init:
                id = getId();
                
                // Only global or local
                if (id < GlobalStart) {
                    _error = Error::OnlyMemAddressesAllowed;
                    _errorAddr = _pc - 1;
                    return -1;
                }
                
                rhs = _stack[--_sp];
                if (id < LocalStart) {
                    addr = id - GlobalStart;
                    memset(_global + addr, _stack[--_sp], rhs * sizeof(uint32_t));
                } else {
                    addr = id - LocalStart + _bp;
                    memset(_stack + addr, _stack[--_sp], rhs * sizeof(uint32_t));
                }
                break;
            case Op::RandomInt: {
                int32_t max = _stack[--_sp];
                int32_t min = _stack[--_sp];
                _stack[_sp++] = uint32_t(random(min, max));
                break;
            }
            case Op::RandomFloat: {
                float max = intToFloat(_stack[--_sp]);
                float min = intToFloat(_stack[--_sp]);
                _stack[_sp++] = floatToInt(random(min, max));
                break;
            }

            case Op::If:
                id = getSz();
                if (_stack[--_sp] == 0) {
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
                _foreachCount = _stack[--_sp];
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
                if (_sp >= _stackSize) {
                    _error = Error::StackOverrun;
                    _errorAddr = _pc - 1;
                    return -1;
                }
                
                targ = (uint16_t(getId()) << 2) | (cmd & 0x03);
                _stack[_sp++] = _pc;
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
                _stack[_sp++] = _pc;

                switch(NativeFunction(id)) {
                    case NativeFunction::None:
                        _error = Error::InvalidNativeFunction;
                        _errorAddr = _pc - 1;
                        return -1;
                    case NativeFunction::LoadColorParam: {
                        setFrame(2, 0);
                        uint32_t r = _stack[_bp];
                        uint32_t i = _stack[_bp + 1];
                        _c[r] = Color(_params[i], _params[i + 1], _params[i + 2]);
                        restoreFrame(0);
                        break;
                    }
                    case NativeFunction::SetAllLights: {
                        setFrame(1, 0);
                        uint32_t r = _stack[_bp];
                        setAllLights(r);
                        restoreFrame(0);
                        break;
                    }
                    case NativeFunction::SetLight: {
                        setFrame(2, 0);
                        uint32_t i = _stack[_bp];
                        uint32_t r = _stack[_bp + 1];
                        setLight(i, _c[r].rgb());
                        restoreFrame(0);
                        break;
                    }
                    case NativeFunction::Animate: {
                        setFrame(1, 0);
                        uint32_t i = _stack[_bp];
                        restoreFrame(animate(i));
                        
                        break;
                    }
                    case NativeFunction::Param: {
                        setFrame(1, 0);
                        uint32_t i = _stack[_bp];
                        restoreFrame(uint32_t(_params[i]));
                        break;
                    }
                    case NativeFunction::LoadColorComp: {
                        setFrame(2, 0);
                        uint32_t c = _stack[_bp];
                        uint32_t i = _stack[_bp + 1];
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
                        restoreFrame(floatToInt(comp));
                        break;
                    }
                    case NativeFunction::StoreColorComp: {
                        setFrame(3, 0);
                        uint32_t c = _stack[_bp];
                        uint32_t i = _stack[_bp + 1];
                        float v = intToFloat(_stack[_bp + 2]);
                        switch(i) {
                            case 0: _c[c].setHue(v); break;
                            case 1: _c[c].setSat(v); break;
                            case 2: _c[c].setVal(v); break;
                            default:
                                _error = Error::InvalidColorComp;
                                _errorAddr = _pc - 1;
                                return -1;
                        }
                        restoreFrame(0);
                        break;
                    }
                    case NativeFunction::Float: {
                        setFrame(1, 0);
                        uint32_t v = _stack[_bp];
                        restoreFrame(floatToInt(float(v)));
                        break;
                    }
                    case NativeFunction::Int: {
                        setFrame(1, 0);
                        float v = intToFloat(_stack[_bp]);
                        restoreFrame(uint32_t(int32_t(v)));
                        break;
                    }
                }
                break;
            case Op::Return: {
                if (_sp == 0) {
                    // Returning from top level is like Exit
                    return 0;
                }
                
                // TOS has return value. Pop it and push it back after restore
                restoreFrame(_stack[--_sp]);
                break;
            }
            case Op::SetFrame:
                getPL(numParams, numLocals);
                setFrame(numParams, numLocals);
                break;

            case Op::Or:    _stack[_sp] |= _stack[--_sp]; break;
            case Op::Xor:   _stack[_sp] ^= _stack[--_sp]; break;
            case Op::And:   _stack[_sp] &= _stack[--_sp]; break;
            case Op::Not:   _stack[_sp] = ~_stack[_sp]; break;
            case Op::LOr:   _stack[_sp++] = _stack[--_sp] || _stack[--_sp]; break;
            case Op::LAnd:  _stack[_sp++] = _stack[--_sp] && _stack[--_sp]; break;
            case Op::LNot:  _stack[_sp] = !_stack[_sp];

            case Op::LTInt:
                rhs = _stack[--_sp];
                _stack[_sp] = int32_t(_stack[_sp]) < int32_t(rhs);
                break;
            case Op::LTFloat:
                rhs = _stack[--_sp];
                _stack[_sp] = intToFloat(_stack[_sp]) < intToFloat(rhs);
                break;
            case Op::LEInt:
                rhs = _stack[--_sp];
                _stack[_sp] = int32_t(_stack[_sp]) <= int32_t(rhs);
                break;
            case Op::LEFloat:
                rhs = _stack[--_sp];
                _stack[_sp] = intToFloat(_stack[_sp]) <= intToFloat(rhs);
                break;
            case Op::EQInt:
                rhs = _stack[--_sp];
                _stack[_sp] = int32_t(_stack[_sp]) == int32_t(rhs);
                break;
            case Op::EQFloat:
                rhs = _stack[--_sp];
                _stack[_sp] = intToFloat(_stack[_sp]) == intToFloat(rhs);
                break;
            case Op::NEInt:
                rhs = _stack[--_sp];
                _stack[_sp] = int32_t(_stack[_sp]) != int32_t(rhs);
                break;
            case Op::NEFloat:
                rhs = _stack[--_sp];
                _stack[_sp] = intToFloat(_stack[_sp]) != intToFloat(rhs);
                break;
            case Op::GEInt:
                rhs = _stack[--_sp];
                _stack[_sp] = int32_t(_stack[_sp]) >= int32_t(rhs);
                break;
            case Op::GEFloat:
                rhs = _stack[--_sp];
                _stack[_sp] = intToFloat(_stack[_sp]) >= intToFloat(rhs);
                break;
            case Op::GTInt:
                rhs = _stack[--_sp];
                _stack[_sp] = int32_t(_stack[_sp]) > int32_t(rhs);
                break;
            case Op::GTFloat:
                rhs = _stack[--_sp];
                _stack[_sp] = intToFloat(_stack[_sp]) > intToFloat(rhs);
                break;

            case Op::AddInt:
                rhs = _stack[--_sp];
                _stack[_sp] = int32_t(_stack[_sp]) + int32_t(rhs);
                break;
            case Op::AddFloat:
                rhs = _stack[--_sp];
                _stack[_sp] = floatToInt(intToFloat(_stack[_sp]) + intToFloat(rhs));
                break;
            case Op::SubInt:
                rhs = _stack[--_sp];
                _stack[_sp] = int32_t(_stack[_sp]) - int32_t(rhs);
                break;
            case Op::SubFloat:
                rhs = _stack[--_sp];
                _stack[_sp] = floatToInt(intToFloat(_stack[_sp]) - intToFloat(rhs));
                break;
            case Op::MulInt:
                rhs = _stack[--_sp];
                _stack[_sp] = int32_t(_stack[_sp]) * int32_t(rhs);
                break;
            case Op::MulFloat:
                rhs = _stack[--_sp];
                _stack[_sp] = floatToInt(intToFloat(_stack[_sp]) * intToFloat(rhs));
                break;
            case Op::DivInt:
                rhs = _stack[--_sp];
                _stack[_sp] = int32_t(_stack[_sp]) / int32_t(rhs);
                break;
            case Op::DivFloat:
                rhs = _stack[--_sp];
                _stack[_sp] = floatToInt(intToFloat(_stack[_sp]) / intToFloat(rhs));
                break;
            case Op::NegInt:
                _stack[_sp] = -int32_t(_stack[_sp]);
                break;
            case Op::NegFloat:
                _stack[_sp] = floatToInt(-intToFloat(_stack[_sp]));
                break;

            case Op::Log:
                // Log tos-i and current addr
                i = getI();
                log(_pc - 1, i, int32_t(_stack[_sp - i]));
                break;
            case Op::LogFloat:
                // Log tos-i and current addr
                i = getI();
                logFloat(_pc - 1, i, intToFloat(_stack[_sp - i]));
                break;
            case Op::LogColor:
                // Log color[i] and current addr
                i = getI();
                logColor(_pc - 1, i, _c[i]);
                break;
        }
    }
}

bool
Interpreter::setFrame(uint8_t params, uint8_t locals)
{
    // FIXME: Add more error checking
    uint16_t savedPC = _stack[--_sp];
    _sp += locals;
    _stack[_sp++] = savedPC;
    _stack[_sp++] = _bp;
    int16_t newBP = _sp - params - locals - 2;
    if (newBP < 0) {
        _errorAddr = _pc - 1;
        _error = Error::MisalignedStack;
        return false;
    }
    _bp = newBP;
    return true;
}

bool
Interpreter::restoreFrame(uint32_t returnValue)
{
    // FIXME: Do some error checking here and return false if so (also check error at call site)
    uint16_t savedBP = _stack[--_sp];
    _pc = _stack[--_sp];
    _sp = _bp;
    _bp = savedBP;
    _stack[_sp++] = returnValue;
    return true;
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
