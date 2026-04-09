/*-------------------------------------------------------------------------
    This source file is a part of PostLightController
    For the latest info, see https://github.com/cmarrin/PostLightController
    Copyright (c) 2021-2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "PostLightController.h"

static const char* TAG = "PostLightController";

static constexpr uint16_t MaxCmdSize = 16;
static constexpr int32_t MaxDelay = 1000; // ms
static constexpr int32_t IdleDelay = 100; // ms

PostLightController::PostLightController(mil::WiFiPortal* portal)
    : mil::Application(portal, ConfigPortalName, true)
{
    mil::System::initLED(1, PixelPin, PixelsPerPost * NumPosts);
}

static int16_t parseCmd(const std::string& cmd, uint8_t* buf, uint16_t size)
{
    // Cmd form is: <cmd char>,<param0 (0-255)>,<param1 (0-255)>,...<paramN (0-255)>
    uint16_t strIndex = 0;
    uint16_t bufIndex = 0;
    bool parsingCmd = true;
    
    while (true) {
        int16_t nextIndex = cmd.find(',', strIndex);
        if (nextIndex == std::string::npos) {
            // HACK ALERT: This is another case where string.length includes the terminating null on espidf!
            nextIndex = strlen(cmd.c_str());
        }

        if (parsingCmd) {
            // Cmd must be a single char
            if (nextIndex != 1) {
                return -1;
            }
            buf[bufIndex++] = uint8_t(cmd.c_str()[0]);
            parsingCmd = false;
        } else {
            // Parse param. Must be 1 to 3 digits <= 255
            uint16_t paramSize = nextIndex - strIndex;
            if (paramSize < 1 || paramSize > 3) {
                return -1;
            }
            
            uint16_t param = 0;
            for (uint16_t i = 0; i < paramSize; ++i) {
                uint16_t digit = uint16_t(cmd.c_str()[strIndex + i]) - '0';
                if (digit > 9) {
                    return -1;
                }
                
                param = param * 10 + digit;
            }
            
            if (param > 255) {
                return -1;
            }
            
            buf[bufIndex++] = uint8_t(param);
        }
        strIndex = nextIndex;
        if (cmd[strIndex] == ',') {
            strIndex += 1;
        }
        
        if (cmd[strIndex] == '\0') {
            return bufIndex;
        }
    }
    return -1;
}

void
PostLightController::processCommand(const std::string& cmd)
{
    mil::System::logI(TAG, "cmd='%s'\n", cmd.c_str());
    
    uint8_t buf[MaxCmdSize];
    int16_t r = parseCmd(cmd, buf, MaxCmdSize);
    if (r < 0) {
        printf("**** parseCmd failed\n");
    } else {
        if (!sendCmd(buf, r)) {
            printf("**** sendCmd failed\n");
        }
    }
    
    _portal->sendHTTPResponse(200, "text/plain", "command processed");
}

void
PostLightController::setup()
{
    mil::System::delay(500);

    showColor(0, 0, 0, 0, 0);

    Application::setup();
    
    // Just in case
    _effect = Effect::None;
    _effectId = -1;

    setTitle((std::string("<center>MarrinTech Post Light Controller v") + Version + "</center>").c_str());

    addHTTPHandler("/command", [this](mil::WiFiPortal* p)
    {
        processCommand(_portal->getHTTPArg("cmd"));
        return true;
    });

    mil::System::logI(TAG, "Post Light Controller v%s", Version);
  
    showStatus(StatusColor::Green, 3, 2);
}
	
void
PostLightController::loop()
{
    Application::loop();

    int32_t delayInMs = IdleDelay;
    
    if (_effect == Effect::Flash) {
        delayInMs = _flash.loop();
    }
    
    if (delayInMs > MaxDelay) {
        delayInMs = MaxDelay;
    }
    
    if (delayInMs < 0) {
        // An effect has finished. End it and clear the display
        showColor(0, 0, 0, 0, 0);
        _effect = Effect::None;
        delayInMs = IdleDelay;
    }
    
    mil::System::delay(delayInMs);
}

bool
PostLightController::sendCmd(const uint8_t* cmd, uint16_t size)
{
    if (size < 1) {
        return false;
    }
    
    if (_effect == Effect::Lua && _effectId >= 0) {
        terminateShellCommand(_effectId);
    }
    
    if (cmd[0] == 'C') {
        // Built-in color command
        showColor(cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
        _effect = Effect::Flash;
        return true;
    }

    _effect = Effect::Lua;
    
    // Make a command with args
    std::string luaCmd = std::string(1, cmd[0]);
    for (int i = 1; i < size; ++i) {
        luaCmd += " " + std::to_string(cmd[i]);
    }
    _effectId = handleShellCommand(luaCmd);
    return true;
}
