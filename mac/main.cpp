/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include <iostream>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <cstdio>

#include "PostLightController.h"

#include "WiFiPortal.h"

// main <.clvx file>

int main(int argc, char * const argv[])
{
    printf("PostLightController Simulator\n\n");
    
    if (argc < 2) {
        printf("No executable given\n");
        return 0;
    }
        
    std::ifstream stream(argv[1], std::ios_base::binary | std::ios_base::ate);
    if (stream.fail()) {
        printf("Can't open '%s'\n", argv[1]);
        return 0;
    }
    
    std::streamsize size = stream.tellg();
    stream.seekg(0, std::ios::beg);
    char* buf = new char[size];
    if (!(stream.read(buf, size))) {
        printf("Error opening '%s'\n", argv[1]);
        return 0;
    }
    
    WiFiPortal portal;
    PostLightController controller(&portal);
    
    controller.setup();
    
    bool haveSentROM = false;
    bool haveSentCmd = false;

    // Loop a few times
    int loopCount = -1;
    while (true) {
        if (loopCount == 0) {
            break;
        }
        else if (loopCount > 0) {
            --loopCount;
        }
        
        controller.loop();
        
        if (controller.isIdle()) {
            if (!haveSentROM) {
                haveSentROM = true;
                controller.uploadFile("/executable.clvx", reinterpret_cast<uint8_t*>(buf), size);
            }
            
            if (!haveSentCmd) {
                haveSentCmd = true;
                controller.processCommand("f,25,200,100,7");
                loopCount = 20;
            }
        }
    }
                        
    return 1;
}
