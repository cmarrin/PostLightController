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

bool
HTTPPathHandler::handle(WebServer& server, HTTPMethod method, const String& uri)
{
    if (method == HTTP_GET) {
        // Handle an operation like list
        String s = "Handled GET: filename='";
        s += uri;
        s += "', op='";
        s+= server.arg("op");
        s+= "'";
        cout << F("***** ") << s.c_str() << F("\n");
        server.send(200, "text/html", s.c_str());
        return true;
    }
    
    server.send(200);  // all done.
    return true;
}

void
HTTPPathHandler::upload(WebServer& server, const String &uri, HTTPUpload &upload)
{
    if (upload.status == UPLOAD_FILE_WRITE) {
        cout << F("Received executable '") << upload.filename << F("'\n");
        _controller->uploadExecutable(upload.buf, upload.currentSize);
    }
}

static uint8_t getCodeByte(void* data, uint16_t addr)
{
    PostLightController* self = reinterpret_cast<PostLightController*>(data);
    return self->getCodeByte(addr);
}

PostLightController::PostLightController()
    : mil::Application(LED_BUILTIN, ConfigPortalName)
    , _pixels(NumPixels, LEDPin)
    , _pathHandler(this, "/fs")
    , _interpretedEffect(&_pixels, ::getCodeByte, this)
{
    memset(_executable, 0, MaxExecutableSize);
}

bool
PostLightController::uploadExecutable(const uint8_t* buf, uint16_t size)
{
    cout << F("Uploading executable, size=") << size << F("...\n");
    File f = LittleFS.open("/executable.clvx", "w");
    if (!f) {
        cout << F("***** failed to open '/executable.clvx' for write\n");
    } else {
        size_t r = f.write(buf, size);
        if (r != size) {
            cout << F("***** failed to write 'executable.clvx', error=") << uint32_t(r) << F("\n");
        } else {
            cout << F("    upload complete.\n");
        }
    }
    f.close();
    return true;
}

static int16_t parseCmd(const String& cmd, uint8_t* buf, uint16_t size)
{
    // Cmd form is: <cmd char>,<param0 (0-255)>,<param1 (0-255)>,...<paramN (0-255)>
    uint16_t strIndex = 0;
    uint16_t bufIndex = 0;
    bool parsingCmd = true;
    
    while (true) {
        int16_t nextIndex = cmd.indexOf(',', strIndex);
        if (nextIndex == -1) {
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
PostLightController::processCommand(const String& cmd)
{
    cout << F("cmd='") << cmd << F("'\n");
    
    uint8_t buf[MaxCmdSize];
    int16_t r = parseCmd(cmd, buf, MaxCmdSize);
    if (r < 0) {
        cout << F("**** parseCmd failed\n");
    } else {
        if (!sendCmd(buf, r)) {
            cout << F("**** sendCmd failed\n");
        }
    }
    
    sendHTTPPage("command processed");
}

void
PostLightController::handleCommand()
{
    processCommand(getHTTPArg("cmd"));
}

static void showError(clvr::Memory::Error error, int16_t addr)
{
    cout << F("Interpreter failed: ");
    
    switch(error) {
        case clvr::Memory::Error::None:                     cout << F("no error"); break;
        case clvr::Memory::Error::InternalError:            cout << F("internal error"); break;
        case clvr::Memory::Error::UnexpectedOpInIf:         cout << F("unexpected op in if (internal error)"); break;
        case clvr::Memory::Error::InvalidOp:                cout << F("invalid opcode"); break;
        case clvr::Memory::Error::OnlyMemAddressesAllowed:  cout << F("only Mem addresses allowed"); break;
        case clvr::Memory::Error::StackOverrun:             cout << F("stack overrun"); break;
        case clvr::Memory::Error::StackUnderrun:            cout << F("stack underrun"); break;
        case clvr::Memory::Error::StackOutOfRange:          cout << F("stack access out of range"); break;
        case clvr::Memory::Error::AddressOutOfRange:        cout << F("address out of range"); break;
        case clvr::Memory::Error::InvalidModuleOp:          cout << F("invalid operation in module"); break;
        case clvr::Memory::Error::InvalidUserFunction:      cout << F("invalid user function"); break;
        case clvr::Memory::Error::ExpectedSetFrame:         cout << F("expected SetFrame as first function op"); break;
        case clvr::Memory::Error::NotEnoughArgs:            cout << F("not enough args on stack"); break;
        case clvr::Memory::Error::WrongNumberOfArgs:        cout << F("wrong number of args"); break;
        case clvr::Memory::Error::InvalidSignature:         cout << F("invalid signature"); break;
        case clvr::Memory::Error::InvalidVersion:           cout << F("invalid version"); break;
        case clvr::Memory::Error::WrongAddressSize:         cout << F("wrong address size"); break;
        case clvr::Memory::Error::NoEntryPoint:             cout << F("invalid entry point in executable"); break;
        case clvr::Memory::Error::NotInstantiated:          cout << F("Need to call instantiate, then construct"); break;
        case clvr::Memory::Error::ImmedNotAllowedHere:      cout << F("immediate not allowed here"); break;
    }
    
    if (addr >= 0) {
        cout << F(" at addr ") << addr;
    }
    
    cout << "\n\n";
}

void
PostLightController::setup()
{
    delay(500);
    Application::setup();
    randomSeed(millis());

    if (!LittleFS.begin(true)) {
        cout << F("***** LittleFS initialization failed\n");
    }

    setTitle("<center>MarrinTech Post Light Controller</center>");

    addHTTPHandler("/command", std::bind(&PostLightController::handleCommand, this));
    addCustomHTTPHandler(&_pathHandler);

    _pixels.begin(); // This initializes the NeoPixel library.
    _pixels.setBrightness(255);
    
    cout << F("Post Light Controller v0.3\n");
  
    showStatus(StatusColor::Green, 3, 2);
    
    // Load the executable
    cout << F("Loading executable...\n");
    File f = LittleFS.open("/executable.clvx", "r");
    if (!f) {
        cout << F("***** failed to open 'executable.clvx' for read\n");
    } else {
        size_t size = f.size();
        size_t r = f.read(_executable, size);
        if (r != size) {
            cout << F("***** failed to read 'executable.clvx', error=") << uint32_t(r) << F("\n");
        } else {
            cout << F("    load complete.\n");
        }
    }
    f.close();
    
}
	
void
PostLightController::loop()
{
    Application::loop();

    int32_t delayInMs = 0;
    
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
        delayInMs = 0;
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
