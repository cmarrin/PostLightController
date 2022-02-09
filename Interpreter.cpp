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
        uint8_t r = 0;
        if (cmd >= 0x80) {
            r = cmd & 0x03;
            cmd &= 0xfc;
        }
        
        uint8_t id;
        uint8_t rd, rs, i;
        uint8_t index;
        uint16_t targ;
        uint16_t addr;
        uint8_t numParams;
        uint8_t numLocals;
        
        switch(Op(cmd)) {
			default:
				_error = Error::InvalidOp;
                _errorAddr = _pc - 1;
				return -1;
            case Op::MinInt        :
                _v[0] = min(int32_t(_v[0]), int32_t(_v[1]));
                break;
            case Op::MinFloat      :
                _v[0] = min(intToFloat(_v[0]), intToFloat(_v[1]));
                break;
            case Op::MaxInt        :
                _v[0] = max(int32_t(_v[0]), int32_t(_v[1]));
                break;
            case Op::MaxFloat      :
                _v[0] = floatToInt(fmaxf(intToFloat(_v[0]), intToFloat(_v[1])));
                break;
            case Op::Init          :
                id = getId();
                
                // Only global or local
                if (id < GlobalStart) {
                    _error = Error::OnlyMemAddressesAllowed;
                    _errorAddr = _pc - 1;
                    return -1;
                }
                
                if (id < LocalStart) {
                    addr = id - GlobalStart;
                    memset(_global + addr, _v[0], _v[1] * sizeof(uint32_t));
                } else {
                    addr = id - LocalStart + _bp;
                    memset(_stack + addr, _v[0], _v[1] * sizeof(uint32_t));
                }
                break;
            case Op::RandomInt     :
                _v[0] = random(int32_t(_v[0]), int32_t(_v[1]));
                break;
            case Op::RandomFloat   :
                _v[0] = floatToInt(random(intToFloat(_v[0]), intToFloat(_v[1])));
                break;
            case Op::Or            :
                _v[0] |= _v[1];
                break;
            case Op::Xor           :
                _v[0] ^= _v[1];
                break;
            case Op::And           :
                _v[0] &= _v[1];
                break;
            case Op::Not           :
                _v[0] = ~_v[0];
                break;
            case Op::LOr           :
                _v[0] = _v[0] || _v[1];
                break;
            case Op::LAnd          :
                _v[0] = _v[0] && _v[1];
                break;
            case Op::LNot          :
                _v[0] = !_v[0];
                break;
            case Op::LTInt         :
                _v[0] = int32_t(_v[0]) < int32_t(_v[1]);
                break;
            case Op::LTFloat       :
                _v[0] = intToFloat(_v[0]) < intToFloat(_v[1]);
                break;
            case Op::LEInt         :
                _v[0] = int32_t(_v[0]) <= int32_t(_v[1]);
                break;
            case Op::LEFloat       :
                _v[0] = intToFloat(_v[0]) <= intToFloat(_v[1]);
                break;
            case Op::EQInt         :
                _v[0] = int32_t(_v[0]) == int32_t(_v[1]);
                break;
            case Op::EQFloat       :
                _v[0] = intToFloat(_v[0]) == intToFloat(_v[1]);
                break;
            case Op::NEInt         :
                _v[0] = int32_t(_v[0]) != int32_t(_v[1]);
                break;
            case Op::NEFloat       :
                _v[0] = intToFloat(_v[0]) != intToFloat(_v[1]);
                break;
            case Op::GEInt         :
                _v[0] = int32_t(_v[0]) >= int32_t(_v[1]);
                break;
            case Op::GEFloat       :
                _v[0] = intToFloat(_v[0]) >= intToFloat(_v[1]);
                break;
            case Op::GTInt         :
                _v[0] = int32_t(_v[0]) > int32_t(_v[1]);
                break;
            case Op::GTFloat       :
                _v[0] = intToFloat(_v[0]) > intToFloat(_v[1]);
                break;
            case Op::AddInt        :
                _v[0] = int32_t(_v[0]) + int32_t(_v[1]);
                break;
            case Op::AddFloat      :
                _v[0] = floatToInt(intToFloat(_v[0]) + intToFloat(_v[1]));
                break;
            case Op::SubInt        :
                _v[0] = int32_t(_v[0]) - int32_t(_v[1]);
                break;
            case Op::SubFloat      :
                _v[0] = floatToInt(intToFloat(_v[0]) - intToFloat(_v[1]));
                break;
            case Op::MulInt        :
                _v[0] = int32_t(_v[0]) * int32_t(_v[1]);
                break;
            case Op::MulFloat      :
                _v[0] = floatToInt(intToFloat(_v[0]) * intToFloat(_v[1]));
                break;
            case Op::DivInt        :
                _v[0] = int32_t(_v[0]) / int32_t(_v[1]);
                break;
            case Op::DivFloat      :
                _v[0] = floatToInt(intToFloat(_v[0]) / intToFloat(_v[1]));
                break;
            case Op::NegInt        :
                _v[0] = -int32_t(_v[0]);
                break;
            case Op::NegFloat      :
                _v[0] = floatToInt(-intToFloat(_v[0]));
                break;
            case Op::LoadColorParam:
                getI(i);
                _c[r] = Color(_params[i], _params[i + 1], _params[i + 2]);
                break;
            case Op::LoadIntParam  :
                getRdRsI(rd, rs, i);
                _v[rd] = _params[i];
                break;
            case Op::LoadFloatParam:
                getRdRsI(rd, rs, i);
                _v[rd] = floatToInt(float(_params[i]));
                break;
            case Op::Load          :
                id = getId();
                _v[r] = loadInt(id, 0);
                break;
            case Op::Store         :
                id = getId();
                storeInt(id, 0, _v[r]);
                break;
                
            case Op::LoadRef       :
                _v[r] = getId();
                break;

            case Op::LoadRefX      :
                id = getId();
                getRdRsI(rd, rs, i);
                index = _v[rs] * i;
                _v[rd] = id + index;
                break;

            case Op::LoadDeref      :
                getRdRsI(rd, rs, i);
                index = _v[rs] + i;
                _v[rd] = loadInt(index);
                break;
            case Op::StoreDeref    :
                getRdRsI(rd, rs, i);
                index = _v[rd] + i;
                storeInt(index, _v[rs]);
                break;
                
            case Op::MoveColor     :
                getRdRs(rd, rs);
                _c[rd] = _c[rs];
                break;
            case Op::Move          :
                getRdRs(rd, rs);
                _v[rd] = _v[rs];
                break;
            case Op::LoadColorComp : {
                getRdRsI(rd, rs, i);
                float f = 0;
                switch(i) {
                    case 0: f = _c[rs].hue(); break;
                    case 1: f = _c[rs].sat(); break;
                    case 2: f = _c[rs].val(); break;
                    default:
                        _error = Error::InvalidColorComp;
                        _errorAddr = _pc - 2;
                        return -1;
                }
                        
                _v[rd] = floatToInt(f);
                break;
            }
            case Op::StoreColorComp : {
                getRdRsI(rd, rs, i);
                float f = intToFloat(_v[rs]);
                switch(i) {
                    case 0: _c[rd].setHue(f); break;
                    case 1: _c[rd].setSat(f); break;
                    case 2: _c[rd].setVal(f); break;
                    default:
                        _error = Error::InvalidColorComp;
                        _errorAddr = _pc - 2;
                        return -1;
                }
                break;
            }
            case Op::LoadBlack     :
                _c[r] = Color();
                break;
            case Op::LoadZero      :
                _v[r] = 0;
                break;
            case Op::LoadIntConst  :
                _v[r] = getConst();
                break;
            case Op::Exit          :
                return _v[r];
            case Op::Call          :
                if (_sp >= _stackSize) {
                    _error = Error::StackOverrun;
                    _errorAddr = _pc - 1;
                    return -1;
                }
                
                targ = (uint16_t(getId()) << 2) | r;
                _stack[_sp++] = _pc;
                _pc = targ + _codeOffset;
                
                if (Op(getUInt8ROM(_pc)) != Op::SetFrame) {
                    _error = Error::ExpectedSetFrame;
                    _errorAddr = _pc - 1;
                    return -1;
                }
                break;
            case Op::CallNative    :
                // FIXME: Probably better this be function pointers
                id = getId();

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
                        restoreFrame();
                        break;
                    }
                    case NativeFunction::SetAllLights: {
                        setFrame(1, 0);
                        uint32_t r = _stack[_bp];
                        setAllLights(r);
                        restoreFrame();
                        break;
                    }
                    case NativeFunction::SetLight: {
                        setFrame(2, 0);
                        uint32_t i = _stack[_bp];
                        uint32_t r = _stack[_bp + 1];
                        setLight(i, _c[r].rgb());
                        restoreFrame();
                        break;
                    }
                    case NativeFunction::Animate: {
                        setFrame(1, 0);
                        uint32_t i = _stack[_bp];
                        _v[0] = animate(i);
                        restoreFrame();
                        break;
                    }
                    case NativeFunction::Param: {
                        setFrame(1, 0);
                        uint32_t i = _stack[_bp];
                        _v[0] = uint32_t(_params[i]);
                        restoreFrame();
                        break;
                    }
                    case NativeFunction::LoadColorComp: {
                        setFrame(2, 0);
                        uint32_t c = _stack[_bp];
                        uint32_t i = _stack[_bp + 1];
                        switch(i) {
                            case 0: _v[0] = _c[c].hue(); break;
                            case 1: _v[0] = _c[c].sat(); break;
                            case 2: _v[0] = _c[c].val(); break;
                            default:
                                _error = Error::InvalidColorComp;
                                _errorAddr = _pc - 1;
                                return -1;
                        }
                        restoreFrame();
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
                        restoreFrame();
                        break;
                    }
                    case NativeFunction::Float: {
                        setFrame(1, 0);
                        uint32_t v = _stack[_bp];
                        _v[0] = floatToInt(float(v));
                        restoreFrame();
                        break;
                    }
                    case NativeFunction::Int: {
                        setFrame(1, 0);
                        float v = intToFloat(_stack[_bp]);
                        _v[0] = uint32_t(int32_t(v));
                        restoreFrame();
                        break;
                    }
                }
                break;
            case Op::Return        :
                if (_sp == 0) {
                    // Returning from top level is like Exit
                    return 0;
                }
                restoreFrame();
                break;
            case Op::SetFrame        :
                getPL(numParams, numLocals);
                setFrame(numParams, numLocals);
                break;
            case Op::Push          :
                _stack[_sp++] = _v[0];
                break;
            case Op::Pop           :
                _v[0] = _stack[_sp++];
                break;
            case Op::ToFloat       :
                _v[r] = floatToInt(float(_v[r]));
                break;
            case Op::ToInt         :
                _v[r] = int32_t(intToFloat(_v[r]));
                break;
            case Op::ForEach       :
                if (_foreachSz >= 0) {
                    // Can't do nested for loops
                    _error = Error::NestedForEachNotAllowed;
                    _errorAddr = _pc - 1;
                    return -1;
                }
                
                _foreachIReg = r;
                _foreachSz = getSz();
                _foreachCount = _v[0];
                _foreachLoopAddr = _pc;
                
                // See if we've already gone past count
                if (_v[_foreachIReg] >= _foreachCount) {
                    _pc += _foreachSz + 1;
                    _foreachSz = -1;
                }
                break;
            case Op::EndForEach    :
                // End of loop. Iterate and check for end
                if (++_v[_foreachIReg] < _foreachCount) {
                    _pc = _foreachLoopAddr;
                } else {
                    // Finished loop
                    _foreachSz = -1;
                }
                break;
            case Op::If            :
                id = getSz();
                if (_v[0] == 0) {
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
            case Op::Else          :
                // If we get here the corresponding If succeeded so ignore this
                _pc += getSz();
                break;
            case Op::EndIf         :
                // This is the end of an if, always ignore it
                break;
            case Op::End           :
                // This is the end of init or loop. If we hit
                // this there was no return. Just do a return 0
                return 0;
            case Op::Log           :
                // Log r and current addr
                log(_pc - 1, r, int32_t(_v[r]));
                break;
            case Op::LogFloat      :
                // Log r and current addr
                logFloat(_pc - 1, r, intToFloat(_v[r]));
                break;
            case Op::LogColor           :
                // Log color and current addr
                logColor(_pc - 1, r, _c[r]);
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
Interpreter::restoreFrame()
{
    // FIXME: Do some error checking here and return false if so (also check error at call site)
    uint16_t savedBP = _stack[--_sp];
    _pc = _stack[--_sp];
    _sp = _bp;
    _bp = savedBP;
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
