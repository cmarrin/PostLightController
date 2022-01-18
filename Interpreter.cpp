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
    _params = buf;
    _paramsSize = size;
    
    _constOffset = 8;
    
    // Find command
    uint32_t constSize = uint32_t(getUInt8ROM(4)) * 4;
    bool found = false;
    uint16_t index = _constOffset + constSize;
    
    while (1) {
        uint8_t c = rom(index);
        if (c == 0) {
            index++;
            break;
        }
        if (c == cmd) {
            // found cmd
            _numParams = getUInt8ROM(index + 1);
            _initStart = getUInt16ROM(index + 2);
            _loopStart = getUInt16ROM(index + 4);
            
            found = true;
            
            //Need to keep looping through all effects
            // to find where the code starts
        }
        
        index += 6;
    }
    
    if (!found) {
        _error = Error::CmdNotFound;
        _errorAddr = -1;
        return false;
    }

    _initStart += index;
    _loopStart += index;
    
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
        uint8_t cmd = getUInt8ROM(_pc++);
        uint8_t r = 0;
        if (cmd >= 0x80) {
            r = cmd & 0x03;
            cmd &= 0xfc;
        }
        
        uint8_t id;
        uint8_t rd, rs, i;
        uint8_t index;
        
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
            case Op::SetLight      :
                getRdRs(rd, rs);
                setLight(_v[rd], _c[rs].rgb());
                break;
            case Op::Init          :
                id = getId();
                
                // Only RAM
                if (id < 0x80) {
                    _error = Error::OnlyMemAddressesAllowed;
                    _errorAddr = _pc - 1;
                    return -1;
                }
                memset(_ram + id - 0x80, _v[0], _v[1] * sizeof(uint32_t));
                break;
            case Op::Random        :
                _v[0] = floatToInt(randomFloat(_v[0], _v[1]));
                break;
            case Op::Bor           :
                _v[0] |= _v[1];
                break;
            case Op::Bxor          :
                _v[0] ^= _v[1];
                break;
            case Op::Band          :
                _v[0] &= _v[1];
                break;
            case Op::Bnot          :
                _v[0] = ~_v[0];
                break;
            case Op::Or            :
                _v[0] = _v[0] || _v[1];
                break;
            case Op::And           :
                _v[0] = _v[0] && _v[1];
                break;
            case Op::Not           :
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
                getI(i);
                _v[r] = _params[i];
                break;
            case Op::LoadFloatParam:
                getI(i);
                _v[r] = floatToInt(float(_params[i]));
                break;
            case Op::LoadColor     :
                id = getId();
                _c[r] = Color(getFloat(id, 0), getFloat(id, 0 + 1), getFloat(id, 0 + 2));
                break;
            case Op::Load          :
                id = getId();
                _v[r] = getInt(id, 0);
                break;
            case Op::StoreColor    :
                id = getId();
                storeColor(id, 0, r);
                break;
            case Op::Store         :
                id = getId();
                storeInt(id, 0, _v[r]);
                break;
            case Op::LoadColorX    :
                id = getId();
                getRdRs(rd, rs);
                index = _v[rs] * 3;
                _c[rd] = Color(getFloat(id, index), getFloat(id, index + 1), getFloat(id, index + 2));
                break;
            case Op::LoadX         :
                id = getId();
                getRdRsI(rd, rs, i);
                index = _v[rs] + i;
                _v[rd] = getInt(id, index);
                break;
            case Op::StoreColorX   :
                id = getId();
                getRdRs(rd, rs);
                storeColor(id, _v[rd] * 3, rs);
                break;
            case Op::StoreX        :
                id = getId();
                getRdRsI(rd, rs, i);
                index = _v[rd] + i;
                storeInt(id, index, _v[rs]);
                break;
            case Op::MoveColor     :
                getRdRs(rd, rs);
                _c[rd] = _c[rs];
                break;
            case Op::Move          :
                getRdRs(rd, rs);
                _v[rd] = _v[rs];
                break;
            case Op::LoadVal       :
                getRdRs(rd, rs);
                _v[rd] = floatToInt(_c[rs].val());
                break;
            case Op::StoreVal      :
                getRdRs(rd, rs);
                _c[rd].setVal(intToFloat(_v[rs]));
                break;
            case Op::LoadBlack     :
                _c[r] = Color();
                break;
            case Op::LoadZero      :
                _v[r] = 0;
                break;
            case Op::LoadIntOne    :
                _v[r] = 1;
                break;
            case Op::LoadFloatOne  :
                _v[r] = floatToInt(1.0f);
                break;
            case Op::LoadByteMax   :
                _v[r] = 255;
                break;
            case Op::Return        :
                return _v[r];
                break;
            case Op::ToFloat       :
                _v[r] = floatToInt(float(_v[r]));
                break;
            case Op::ToInt         :
                _v[r] = int32_t(floatToInt(_v[r]));
                break;
            case Op::SetAllLights  : {
                uint32_t rgb = _c[r].rgb();
                uint8_t n = numPixels();
                for (uint8_t i = 0; i < n; ++i) {
                    setLight(i, rgb);
                }
                break;
            }
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
                log(_pc - 1, r, intToFloat(_v[r]));
                break;
            case Op::LogColor           :
                // Log color and current addr
                logColor(_pc - 1, r, _c[r]);
                break;
        }
    }
}
