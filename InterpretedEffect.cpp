/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "InterpretedEffect.h"

#include "mil.h"

void
InterpretedEffect::setLight(uint16_t id, clvr::InterpreterBase* interp, void* data)
{
    // First arg is byte index of light to set. Next is a ptr to a struct of
    // h, s, v byte values (0-255)
    uint8_t i = interp->memMgr()->getArg(2, clvr::VarArgSize);
    clvr::AddrNativeType addr = interp->memMgr()->getArg(clvr::VarArgSize + 2, clvr::AddrSize);
    uint8_t h = interp->memMgr()->getAbs(addr, 1);
    uint8_t s = interp->memMgr()->getAbs(addr + 1, 1);
    uint8_t v = interp->memMgr()->getAbs(addr + 2, 1);
    
    InterpretedEffect* effect = reinterpret_cast<InterpretedEffect*>(data);
    effect->_pixels->setPixelColor(i, effect->_pixels->color(h, s, v));
}

bool
InterpretedEffect::init(uint8_t cmd, const uint8_t* buf, uint32_t size)
{
    _interp.instantiate();
    if (_interp.error() != clvr::Memory::Error::None) {
        return false;
    }
    
    _interp.addUserFunction(3, setLight, this);

    for (int i = size - 1; i >= 0; --i) {
        _interp.addArg(buf[i], clvr::Type::UInt8);
    }
    _interp.addArg(cmd, clvr::Type::UInt8); // cmd
    _interp.construct();
    _interp.dropArgs(size + 1);
    
    if (_interp.error() != clvr::Memory::Error::None) {
        return false;
    }
    
    _interp.interp(MyInterpreter::ExecMode::Start);

	if (_interp.error() != clvr::Memory::Error::None) {
		return false;
	}

    cout << F("InterpretedEffect started: cmd='");
    cout << char(cmd);
    cout << F("'");

	return true;
}

int32_t
InterpretedEffect::loop()
{
    uint32_t result = _interp.interp(MyInterpreter::ExecMode::Start);
    return (_interp.error() != clvr::Memory::Error::None) ? -1 : result;
}
