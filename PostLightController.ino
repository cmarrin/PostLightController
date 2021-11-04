// Copyright Chris Marrin 2021

//
// Pond Light Controller
//
// Each controller is a 5V Arduino Nano with serial data coming in on pin D5 and data out to
// an 8 pixel WS2812 ring on D6. Serial data, +5V and Gnd comes into each post and goes out to
// the next post.
//
// Serial Protocol
//
// Standard RS232 protocol is used with 1 start bit, 8 data bits and 1 stop bit. Bit rate is
// set by the protocol from 1200 bps to 57600 bps. When a break at 1200 baud is received
// (10ms minimum duration) baud rate is reset to 1200 baud. When a set baud rate command
// with an address of 0 (all devices) is received, the baud rate is set to that rate. Valid
// rates are 1200, 2400, 4800, 9600, 14400, 19200, 38400 and 57600.
//
// Each incoming command consists of 4 bytes: Address (0 = all devices), Command and 2 Data
// bytes. There are 32 commands each command byte contains a 3 bit data value, for a total of 
// 19 bits of data.
//
// Command List:

//    Command 	Name         	Params						Description
//    -------   ----            ------    					-----------
//    	0       Reset           x         					Reset to all devices off, 1200 baud
//
//      1       Set Baud        rate      					3 bit command data is rate, data bytes ignored
//
//      2       Constant Color  color						Set lights to passed color
//
//		3		Flicker			color, speed				Flicker lights based on passed color at 
//															passed speed (0-7).
//
//		4		Throb			color, speed, depth			Throb lights with passed color, dimming at passed
//															speed (0-7). Passed depth is amount of dimming
//															0 (slight dimming) to 7 (dimming to 0 brightness)
//
//		5		Rainbow			color, speed, range, mode	Change color through the rainbow by changing hue 
//															from the passed color at the passed speed. Range
//															is how far from the passed color to go, 0 (very little)
//															to 7 (full range). If mode (1 bit) is 0 colors bounce 
//															back and forth between start color through range. If
//															it is 1 colors cycle the full color spectrum and
//															range is ignored.

// Params is combined into a single uint32_t with the lower 19 bits being used. Bits 0-7 are 
// the second data byte, 8-15 are the first data byte and 16-18 are the 3 data bits in the
// command byte.

// Color is the 11 lowest bits of params. bits 6-10 are the hue (0-31), bits 3-5 are the 
// saturation (0-7) and bits 0-2 are the value (0-7). Other params are in higher order bits
// according to the param list

#include <Adafruit_NeoPixel.h>

#include "Flicker.h"

Flicker* flicker = nullptr;

static constexpr uint32_t speed = 2; // value from 0 to 7, will eventually be passed in
static constexpr uint8_t hue = 3;
static constexpr uint8_t sat = 7;
static constexpr uint8_t val = 4;

constexpr int PIN = 6;
constexpr int NUMPIXELS = 8;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
    Serial.begin(115200);
    Serial.print("Hello\n");

    pixels.begin(); // This initializes the NeoPixel library.
    pixels.setBrightness(255);
	
	flicker = new Flicker(&pixels);
	
	uint32_t param = Effect::HSVParamsToPackedHSV(hue, sat, val);
	param |= speed << 11;
	flicker->init(param);
}

void loop()
{
	uint32_t delayInMs = flicker->loop();
	
	// Eventually we can't delay in loop because we have to feed the soft serial port
	// But for now...
	delay(delayInMs);
}
