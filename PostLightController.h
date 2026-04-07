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

#pragma once

#include "Application.h"
#include "Flash.h"

static constexpr const char* ConfigPortalName = "MT PostLightController";
static constexpr const char* Hostname = "plc";
static constexpr const char* Version = "0.1";

static constexpr int PixelsPerPost = 8;
static constexpr int NumPosts = 7;
static constexpr int PixelPin = 10;
static constexpr int TotalPixels = PixelsPerPost * NumPosts;

class PostLightController : public mil::Application
{
  public:
    PostLightController(mil::WiFiPortal*);

    virtual void setup() override;
    virtual void loop() override;
    
    bool sendCmd(const uint8_t* cmd, uint16_t size);
    void processCommand(const std::string& cmd);

  private:	
	enum class StatusColor { Red, Green, Yellow, Blue };

	void showColor(uint8_t h, uint8_t s, uint8_t v, uint8_t n, uint8_t d)
	{
        // FIXME: terminate any Lua program running
        
        _effect = Effect::Flash;
        _flash.init(h, s, v, n, d);
	}
	
	void showStatus(StatusColor color, uint8_t numberOfBlinks = 0, uint8_t interval = 0)
	{
		// Flash half bright red or green at passed interval and passed number of times
        uint8_t h = 128;
        
        switch (color) {
            case StatusColor::Red: h = 0; break;
            case StatusColor::Green: h = 85; break;
            case StatusColor::Yellow: h = 30; break;
            case StatusColor::Blue: h = 140; break;
        }
        showColor(h, 0xff, 0x80, numberOfBlinks, interval);
	}
 
    enum class Effect { None, Flash, Lua };
    Effect _effect = Effect::None;
	Flash _flash;
};
