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

#include "lua.hpp"

// These must match values in Clover code
static constexpr int SetLight = 1;
static constexpr int SetLights = 2;
static constexpr int ShowLights = 3;

void
InterpretedEffect::userCall(uint16_t id, clvr::InterpreterBase* interp, void* data)
{
    InterpretedEffect* effect = reinterpret_cast<InterpretedEffect*>(data);

    if (id == ShowLights) {
        effect->_pixels->show();
        return;
    }
    
    clvr::AddrNativeType addr = 0;
    uint8_t i = 0;
    uint8_t from = 0;
    uint8_t count = 0;
    
    if (id == SetLight) {
        // First arg is byte index of light to set. Next is a ptr to a struct of
        // h, s, v byte values (0-255)
        i = interp->memMgr()->getArg(2, clvr::VarArgSize);
        addr = interp->memMgr()->getArg(clvr::VarArgSize + 2, clvr::AddrSize);
    } else {
        // First arg is byte index of the first light to set. Next is the number
        // of consecutive lights to set. Next is a ptr to a struct of
        // h, s, v byte values (0-255)
        from = interp->memMgr()->getArg(2, clvr::VarArgSize);
        count = interp->memMgr()->getArg(clvr::VarArgSize + 2, clvr::VarArgSize);
        addr = interp->memMgr()->getArg(clvr::VarArgSize * 2 + 2, clvr::AddrSize);
    }
    
    uint8_t h = interp->memMgr()->getAbs(addr, 1);
    uint8_t s = interp->memMgr()->getAbs(addr + 1, 1);
    uint8_t v = interp->memMgr()->getAbs(addr + 2, 1);

    if (id == SetLight) {
        effect->_pixels->setLight(i, effect->_pixels->color(h, s, v));
    } else {
        effect->_pixels->setLights(from, count, effect->_pixels->color(h, s, v));
    }
}

bool
InterpretedEffect::init(uint8_t cmd, const uint8_t* buf, uint32_t size)
{
    lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	std::string sample = "print(\"Hello World from Lua inside c++!\")";
 
 	int err = luaL_loadbuffer(L, sample.c_str(), sample.length(), "mysample");
	if (err) {
		printf("Error initializing lua with hello world script: %i, %s\n", err, lua_tostring(L, -1));
		lua_close(L);
		return(0);
	}

	err = lua_pcall(L, 0, 0, 0);
	if (err) {
		printf("Error running lua hello world script: %i, %s\n", err, lua_tostring(L, -1));
		lua_close(L);
		return(0);
	}

	printf("Success running hello world script\n");
	lua_close(L);

    printf("InterpretedEffect started: cmd='%c'\n", char(cmd));

    _interp.instantiate();
    if (_interp.error() != clvr::Memory::Error::None) {
        return false;
    }
    
    _interp.addUserFunction(SetLight, userCall, this);
    _interp.addUserFunction(SetLights, userCall, this);
    _interp.addUserFunction(ShowLights, userCall, this);

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

	return true;
}

int32_t
InterpretedEffect::loop()
{
    uint32_t result = 0; //_interp.interp(MyInterpreter::ExecMode::Start);
    _pixels->show();
    return (_interp.error() != clvr::Memory::Error::None) ? -1 : result;
}
