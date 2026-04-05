/*-------------------------------------------------------------------------
    This source file is a part of Clover
    For the latest info, see https://github.com/cmarrin/Clover
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Flash Class
//
// This class flashes the lights a given number of times (for error output)

#pragma once

#include <stdint.h>

class Flash
{
public:
	bool init(uint8_t h, uint8_t s, uint8_t v, uint8_t count, uint16_t duration);
	int32_t loop();
		
private:
    uint8_t _red = 0;
    uint8_t _green = 0;
    uint8_t _blue = 0;
	uint8_t _count = 0;
	uint16_t _duration = 1; // in ms
	
	uint32_t _lastFlash = 0;
	bool _on = false;
	uint8_t _countCompleted = 0;
};
