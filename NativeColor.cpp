// Copyright Chris Marrin 2021
//
// NativeColor

#include "NativeColor.h"

#include "Interpreter.h"

using namespace arly;

#ifndef ARDUINO
#include "CompileEngine.h"

void
NativeColor::addFunctions(CompileEngine* comp)
{
    comp->addNative("LoadColorParam", uint8_t(Id::LoadColorParam), CompileEngine::Type::None,
                            CompileEngine::SymbolList {
                                { "c", 0, CompileEngine::Type::Int },
                                { "p", 1, CompileEngine::Type::Int },
                                { "n", 2, CompileEngine::Type::Int },
                            });
    comp->addNative("SetAllLights", uint8_t(Id::SetAllLights), CompileEngine::Type::None,
                            CompileEngine::SymbolList {
                                { "c", 0, CompileEngine::Type::Int },
                            });
    comp->addNative("SetLight", uint8_t(Id::SetLight), CompileEngine::Type::None,
                            CompileEngine::SymbolList {
                                { "i", 0, CompileEngine::Type::Int },
                                { "c", 1, CompileEngine::Type::Int },
                            });
    comp->addNative("LoadColorComp", uint8_t(Id::LoadColorComp), CompileEngine::Type::Float,
                            CompileEngine::SymbolList {
                                { "c", 0, CompileEngine::Type::Int },
                                { "i", 1, CompileEngine::Type::Int },
                            });
    comp->addNative("StoreColorComp", uint8_t(Id::StoreColorComp), CompileEngine::Type::None,
                            CompileEngine::SymbolList {
                                { "c", 0, CompileEngine::Type::Int },
                                { "i", 1, CompileEngine::Type::Int },
                                { "v", 2, CompileEngine::Type::Float },
                            });
}
#endif

bool
NativeColor::hasId(uint8_t id) const 
{
    switch(Id(id)) {
        default:
            return false;
        case Id::LoadColorParam  :
        case Id::SetAllLights    :
        case Id::SetLight        :
        case Id::LoadColorComp   :
        case Id::StoreColorComp  :
            return true;
    }
}

uint8_t
NativeColor::numParams(uint8_t id) const
{
    switch(Id(id)) {
        default                 : return 0;
        case Id::LoadColorParam : return 3;
        case Id::SetAllLights   : return 1;
        case Id::SetLight       : return 2;
        case Id::LoadColorComp  : return 2;
        case Id::StoreColorComp : return 3;
    }
}

int32_t
NativeColor::call(Interpreter* interp, uint8_t id)
{
    switch(Id(id)) {
        default:
            return 0;
        case Id::LoadColorParam      : {
            uint32_t c = interp->stackLocal(0);
            uint32_t p = interp->stackLocal(1);
            uint32_t n = interp->stackLocal(2);
            if (c + n > 4) {
                interp->setError(Interpreter::Error::AddressOutOfRange);
                return 0;
            }
            for ( ; n > 0; ++c, --n, p += 3) {
                _c[c] = Color(interp->param(p), interp->param(p + 1), interp->param(p + 2));
            }
            return 0;
        }
        case Id::SetAllLights        : {
            uint32_t r = interp->stackLocal(0);
            setAllLights(uint8_t(r));
            return 0;
        }
        case Id::SetLight        : {
            uint32_t i = interp->stackLocal(0);
            uint32_t r = interp->stackLocal(1);
            if (_setLightFun) {
                _setLightFun(i, _c[r].rgb());
            }
            return 0;
        }
        case Id::LoadColorComp          : {
            uint32_t c = interp->stackLocal(0);
            uint32_t i = interp->stackLocal(1);
            float comp;
            switch(i) {
                case 0: comp = _c[c].hue(); break;
                case 1: comp = _c[c].sat(); break;
                case 2: comp = _c[c].val(); break;
                default:
                    interp->setError(Interpreter::Error::InvalidColorComp);
                    return 0;
            }
            return floatToInt(comp);
        }
        case Id::StoreColorComp       : {
            uint32_t c = interp->stackLocal(0);
            uint32_t i = interp->stackLocal(1);
            float v = intToFloat(interp->stackLocal(2));
            switch(i) {
                case 0: _c[c].setHue(v); break;
                case 1: _c[c].setSat(v); break;
                case 2: _c[c].setVal(v); break;
                default:
                    interp->setError(Interpreter::Error::InvalidColorComp);
                    break;
            }
            return 0;
        }
    }
}

void
NativeColor::setAllLights(uint8_t r)
{
    if (!_setLightFun) {
        return;
    }
    
    uint32_t rgb = _c[r].rgb();
    uint8_t n = _numPixels;

    for (uint8_t i = 0; i < n; ++i) {
        _setLightFun(i, rgb);
    }
}
