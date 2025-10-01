/*-------------------------------------------------------------------------
    This source file is a part of PostLightController
    For the latest info, see https://github.com/cmarrin/PostLightController
    Copyright (c) 2021-2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "PostLightController.h"

static constexpr uint16_t MaxCmdSize = 16;
static constexpr int32_t MaxDelay = 1000; // ms
static constexpr int32_t IdleDelay = 100; // ms

static uint8_t getCodeByte(void* data, uint16_t addr)
{
    PostLightController* self = reinterpret_cast<PostLightController*>(data);
    return self->getCodeByte(addr);
}

PostLightController::PostLightController(WiFiPortal* portal)
    : mil::Application(portal, LED_BUILTIN, ConfigPortalName)
    , _pixels(TotalPixels, LEDPin)
    , _interpretedEffect(&_pixels, ::getCodeByte, this)
{
    memset(_executable, 0, MaxExecutableSize);
}

bool
PostLightController::uploadHTTPFile(const std::string& uri)
{
    // Get filename, Name is the string past the "/fs" (assume string started with "/fs")
    std::string filename = uri.substr(3);
    size_t size = httpContentLength();
    uint8_t* buf = new uint8_t[size + 1];
    readHTTPContent(buf, size);
    uploadFile(filename, buf, size);
    delete [ ] buf;
    return true;
}

bool
PostLightController::uploadFile(const std::string& filename, const uint8_t* buf, size_t size)
{
    // If this is the executable, show status lights
    bool show = filename == "/executable.clvx";
    
    if (show) {
        showStatus(StatusColor::Blue, 0, 0);
    }
    
    printf("Uploading file, size=%u...\n", (unsigned int) size);
    File f = _wfs.open(filename.c_str(), "w");
    if (!f) {
        printf("***** failed to open '%s' for write\n", filename.c_str());
        if (show) {
            showStatus(StatusColor::Red, 5, 5);
            show = false;
        }
    } else {
        size_t r = f.write(buf, size);
        if (r != size) {
            printf("***** failed to write 'executable.clvx', error=%u\n", (unsigned int) r);
            if (show) {
                showStatus(StatusColor::Red, 5, 5);
                show = false;
            }
        } else {
            printf("    upload complete.\n");
            if (show) {
                showStatus(StatusColor::Green, 5, 1);
            }
        }
    }
    f.close();
    
    if (show) {
        loadExecutable();
    }

    return true;
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
            nextIndex = cmd.length();
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
    printf("cmd='%s'\n", cmd.c_str());
    
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

bool
PostLightController::handleCommand(WiFiPortal* portal, WiFiPortal::HTTPMethod, const std::string& uri)
{
    processCommand(portal->getHTTPArg("cmd"));
    return true;
}

static void showError(clvr::Memory::Error error, int16_t addr)
{
    const char* reason = "";
    
    switch(error) {
        case clvr::Memory::Error::None:                     reason = "no error"; break;
        case clvr::Memory::Error::InternalError:            reason = "internal error"; break;
        case clvr::Memory::Error::UnexpectedOpInIf:         reason = "unexpected op in if (internal error)"; break;
        case clvr::Memory::Error::InvalidOp:                reason = "invalid opcode"; break;
        case clvr::Memory::Error::OnlyMemAddressesAllowed:  reason = "only Mem addresses allowed"; break;
        case clvr::Memory::Error::StackOverrun:             reason = "stack overrun"; break;
        case clvr::Memory::Error::StackUnderrun:            reason = "stack underrun"; break;
        case clvr::Memory::Error::StackOutOfRange:          reason = "stack access out of range"; break;
        case clvr::Memory::Error::AddressOutOfRange:        reason = "address out of range"; break;
        case clvr::Memory::Error::InvalidModuleOp:          reason = "invalid operation in module"; break;
        case clvr::Memory::Error::InvalidUserFunction:      reason = "invalid user function"; break;
        case clvr::Memory::Error::ExpectedSetFrame:         reason = "expected SetFrame as first function op"; break;
        case clvr::Memory::Error::NotEnoughArgs:            reason = "not enough args on stack"; break;
        case clvr::Memory::Error::WrongNumberOfArgs:        reason = "wrong number of args"; break;
        case clvr::Memory::Error::InvalidSignature:         reason = "invalid signature"; break;
        case clvr::Memory::Error::InvalidVersion:           reason = "invalid version"; break;
        case clvr::Memory::Error::WrongAddressSize:         reason = "wrong address size"; break;
        case clvr::Memory::Error::NoEntryPoint:             reason = "invalid entry point in executable"; break;
        case clvr::Memory::Error::NotInstantiated:          reason = "Need to call instantiate, then construct"; break;
        case clvr::Memory::Error::ImmedNotAllowedHere:      reason = "immediate not allowed here"; break;
    }
    
    printf("Interpreter failed: %s", reason);

    if (addr >= 0) {
        printf(" at addr %i", (int) addr);
    }
    
    printf("\n\n");
}

void
PostLightController::loadExecutable()
{
    printf("Loading executable...\n");
    
    File f = _wfs.open("/executable.clvx", "r");
    if (!f) {
        printf("***** failed to open 'executable.clvx' for read\n");
    } else {
        size_t size = f.size();
        size_t r = f.read(_executable, size);
        if (r != size) {
            printf("***** failed to read 'executable.clvx', error=%i\n", (int) r);
        } else {
            printf("    load complete.\n");
        }
    }
    f.close();
}

void
PostLightController::setup()
{
    delay(500);
    Application::setup();
    randomSeed(millis());

    setTitle("<center>MarrinTech Post Light Controller v0.4</center>");

    addHTTPHandler("/command", [this](WiFiPortal* p, WiFiPortal::HTTPMethod m, const std::string& uri) -> bool
    {
        handleCommand(p, m, uri);
        return true;
    });
    
    _pixels.begin(); // This initializes the NeoPixel library.
    _pixels.setBrightness(255);
    
    printf("Post Light Controller v0.4\n");
  
    showStatus(StatusColor::Green, 3, 2);
    
    loadExecutable();
}
	
void
PostLightController::loop()
{
    Application::loop();

    int32_t delayInMs = IdleDelay;
    
    if (_effect == Effect::Flash) {
        delayInMs = _flash.loop(&_pixels);
    } else if (_effect == Effect::Interp) {
        delayInMs = _interpretedEffect.loop();
        if (_interpretedEffect.error() != clvr::Memory::Error::None) {
            _effect = Effect::None;
            showError(_interpretedEffect.error(), _interpretedEffect.errorAddr());
        }
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
    
    delay(delayInMs);
}

bool
PostLightController::sendCmd(const uint8_t* cmd, uint16_t size)
{
    if (size < 1) {
        return false;
    }
    
    if (cmd[0] == 'C') {
        // Built-in color command
        showColor(cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
        return true;
    }
    
    _effect = Effect::Interp;
    _interpretedEffect.init(cmd[0], cmd + 1, size - 1);
    if (_interpretedEffect.error() != clvr::Memory::Error::None) {
        _effect = Effect::None;
        showError(_interpretedEffect.error(), _interpretedEffect.errorAddr());
        return false;
    }
    return true;
}
