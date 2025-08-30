/*-------------------------------------------------------------------------
    This source file is a part of PostLightController
    For the latest info, see https://github.com/cmarrin/PostLightController
    Copyright (c) 2021-2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

/*
Post Light Controller for ESP

Each controller is an ESP32 C3 SuperMini connected to an 8 pixel WS2812 ring on D10
+5V and Gnd comes into each post and goes out to the next post.

Each ESP runs a web server which is used to set its WiFi credentials and its 
hostname. a /command page is then setup where the command and its parameters
are sent to do each lighting effect.

Command Structure

Commands are sent as arguments to the /command URI. Command structure is:

    http://.../command?cmd=c,xx,xx,...
    
where 'c' is t he command to execute (e.g., 'f' for flicker) and the paramaters
to the command are in the comma separated numeric list that follows. Each
number is 0 to 255 and can be decimal or hex if preceded by '0x'.

Command List:

   Command 	Name         	    Params						Description
   -------  ----                ------    					-----------
	'C'     Constant Color 	    color, n, d					Set lights to passed color
                                                            Flash n times, for d duration (in 100ms units)
                                                            If n == 0, just turn lights on

	'X'		Write Executable	addr, <data>				Write EEPROM starting at addr. Data can be
                                                            up to 64 bytes, due to buffering limitations.

    All other commands are a single lower case letter followed by as many params are needed for the command.
    All commands take at least one color param which is 3 consecutive bytes (hue, saturation, value). 
    Saturation and value are levels between 0 and 255. Hue is an angle on the color wheel. A 0-360 degree 
    value is obtained with hue / 256 * 360.
*/

#include "PostLightController.h"

PostLightController controller;

extern "C" {
void app_main(void)
{
    controller.setup();

    while (true) {
        controller.loop();
    }
}
}
