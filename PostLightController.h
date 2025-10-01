/*-------------------------------------------------------------------------
    This source file is a part of PostLightController
    For the latest info, see https://github.com/cmarrin/PostLightController
    Copyright (c) 2021-2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// PostLightController runs on Mac and ESP32 (possibly ESP8266). It uses
// HTTP to send commands and software updates to the unit.

#include "Application.h"
#include "Flash.h"
#include "InterpretedEffect.h"

static constexpr const char* ConfigPortalName = "MT Etherclock";
static constexpr int LEDPin = 10;
static constexpr int PixelsPerPost = 8;
static constexpr int NumPosts = 7;
static constexpr int TotalPixels = PixelsPerPost * NumPosts;
static constexpr int MaxExecutableSize = 2048;

class PostLightController;

class PostLightController : public mil::Application
{
  public:
    PostLightController(WiFiPortal*);

    virtual void setup() override;
    virtual void loop() override;
    
    bool uploadHTTPFile(const std::string& uri);
    bool uploadFile(const std::string& filename, const uint8_t* buf, size_t size);
    bool sendCmd(const uint8_t* cmd, uint16_t size);
    void processCommand(const std::string& cmd);
    
    bool isIdle() const { return _effect == Effect::None; }

    uint8_t getCodeByte(uint16_t addr) { return (addr < MaxExecutableSize) ? _executable[addr] : 0; }
    
  private:	
    bool handleCommand(WiFiPortal*, WiFiPortal::HTTPMethod, const std::string& uri);
    void handleGetIPAddr();
    
    void loadExecutable();

	enum class StatusColor { Red, Green, Yellow, Blue };

	void showColor(uint8_t h, uint8_t s, uint8_t v, uint8_t n, uint8_t d)
	{
        _effect = Effect::Flash;
        _flash.init(&_pixels, h, s, v, n, d);
	}
	
	void showStatus(StatusColor color, uint8_t numberOfBlinks = 0, uint8_t interval = 0)
	{
		// Flash half bright red or green at passed interval and passed number of times
        _effect = Effect::Flash;
        uint8_t h = 128;
        
        switch (color) {
            case StatusColor::Red: h = 0; break;
            case StatusColor::Green: h = 85; break;
            case StatusColor::Yellow: h = 30; break;
            case StatusColor::Blue: h = 140; break;
        }
        _flash.init(&_pixels, h, 0xff, 0x80, numberOfBlinks, interval);
	}

	mil::NeoPixel _pixels;
 
    enum class Effect { None, Flash, Interp };
    Effect _effect = Effect::None;
	Flash _flash;
	InterpretedEffect _interpretedEffect;
 
	uint8_t _cmd = '0';
 
    uint8_t _executable[MaxExecutableSize];
};

