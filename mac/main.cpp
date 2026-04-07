/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "PostLightController.h"

#include "MacWiFiPortal.h"
#include "tigr.h"

#include <numbers>
#include <cmath>

mil::MacWiFiPortal portal;

static const char* TAG = "PostLightController";

static constexpr double PI = std::numbers::pi;
static constexpr int LEDsPerRing = 8;
static constexpr double AnglePerLED = 2 * PI / LEDsPerRing;
static constexpr int LEDRadius = 10;
static constexpr int RingCount = 7;
static constexpr int RingSize = 80;
static constexpr int Spacing = 50;
static constexpr int WindowWidth = RingSize * RingCount + Spacing * (RingCount + 1);
static constexpr int WindowHeight = RingSize + Spacing * 2;

static void polarToCartesian(double angle, int radius, int& x, int& y)
{
    x = std::cos(angle) * radius;
    y = std::sin(angle) * radius;
}

static void colorToRGB(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b)
{
    r = (color >> 16) & 0xff;
    g = (color >> 8) & 0xff;
    b = color & 0xff;
}

static void makeRing(Tigr* screen, const uint32_t* buffer, int centerX, int centerY)
{
    double angle = 0;
    
    for (int i = 0; i < LEDsPerRing; ++i) {
        int x;
        int y;
        polarToCartesian(angle, RingSize / 2, x, y);
        uint32_t color = buffer[i];
        uint8_t r, g, b;
        colorToRGB(color, r, g, b);
        tigrFillCircle(screen, x + centerX, y + centerY, LEDRadius, tigrRGB(r, g, b));
        angle += AnglePerLED;
    }
}

int main(int argc, char * const argv[])
{
    while (true) {
        mil::System::logI(TAG, "Opening tigr window");

        Tigr* screen = tigrWindow(WindowWidth, WindowHeight, "PostLightController", TIGR_AUTO);
        
        mil::System::setRenderCB([screen, &needRender](const mil::Graphics* gfx)
        {
            const uint32_t* b = reinterpret_cast<const uint32_t*>(buffer);
            tigrClear(screen, tigrRGBA(0x0, 0x00, 0x00, 0xff));

            int x = RingSize / 2 + Spacing;
            for (int i = 0; i < RingCount; ++i) {
                makeRing(screen, b + LEDsPerRing * i, x, RingSize / 2 + Spacing);
                x += RingSize + Spacing;
            }
        });

        PostLightController controller(&portal);
        
        controller.setup();
        
        while (!tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE)) {
            if (mil::System::isRestarting()) {
                break;
            }
            
            controller.loop();
            tigrUpdate(screen);
            mil::System::delay(10);
        }

        tigrFree(screen);
    }
    
    return 1;
}
