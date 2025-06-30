/*-------------------------------------------------------------------------
    This source file is a part of PostLightController
    For the latest info, see https://github.com/cmarrin/PostLightController
    Copyright (c) 2021-2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "PostLightController.h"

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
    String s = F("upload (filename='");
    s += upload.filename;
    s += F("' ");
    
    if (upload.status == UPLOAD_FILE_START) {
        s += F("START");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        s += F("WRITE - received ");
        s += uint32_t(upload.currentSize);
        s += F(" bytes:");
        char buf[3] = "00";
        for (uint8_t i = 0; i < 10; ++i) {
            mil::toHex(upload.buf[i], buf);
            s += F(" 0x");
            s += buf;
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        s += F("END");
    }
    
    if (s.length() != 0) {
        cout << s << "\n";
        server.send(200, s.c_str());  // all done.
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
    mil::File f = _wfs.fs().open("executable.clvx", "w");
    f.write(buf, size);
    f.close();
    return true;
}

void
PostLightController::handleCommand()
{
    cout << F("cmd='") << getHTTPArg("cmd") << F("'\n");
    sendHTTPPage("command processed");
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
    
    _effect = Effect::Interp;
    _interpretedEffect.init(cmd[0], cmd + 1, size - 1);
    if (_interpretedEffect.error() != clvr::Memory::Error::None) {
        _effect = Effect::None;
        showError(_interpretedEffect.error(), _interpretedEffect.errorAddr());
        return false;
    }
    return true;
}
