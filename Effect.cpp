// Copyright Chris Marrin 2021
//
// Effect Base Class

#include "Effect.h"

uint16_t Effect::ColorTable[64] = {
	0x000, 0x333, 0x555, 0x888, 0xaaa, 0xccc, 0xeee, 0xfff, 
	0x404, 0x606, 0x909, 0xc0c, 0xf0f, 0xf6f, 0xfaf, 0xfef, 
	0x004, 0x006, 0x009, 0x00c, 0x00f, 0x66f, 0xaaf, 0xeef, 
	0x044, 0x066, 0x099, 0x0cc, 0x0ff, 0x6ff, 0xaff, 0xeff, 
	0x040, 0x060, 0x090, 0x0c0, 0x0f0, 0x6f6, 0xafa, 0xefe, 
	0x440, 0x660, 0x990, 0xcc0, 0xff0, 0xff6, 0xffa, 0xffe, 
	0x420, 0x620, 0x940, 0xc60, 0xf80, 0xf86, 0xfa8, 0xfec, 
	0x400, 0x600, 0x900, 0xc00, 0xf00, 0xf66, 0xfaa, 0xfee, 
};

void
Effect::init(const uint8_t* buf, uint32_t size)
{
	if (size < 5 || !buf) {
		Serial.println("***** Buffer not passed. Resetting...");
		_hue = 0;
		_sat = 0;
		_val = 0;
		_param1 = 0;
		_param2 = 0;
	} else {
		_hue = float(cmdParamToValue(buf[0])) / 64;
		_sat = float(cmdParamToValue(buf[1])) / 64;
		_val = float(cmdParamToValue(buf[2])) / 64;
		_param1 = cmdParamToValue(buf[3]);
		_param2 = cmdParamToValue(buf[4]);
		Serial.print("Effect inited: hue=");
		Serial.print(_hue);
		Serial.print(", sat=");
		Serial.print(_sat);
		Serial.print(", val=");
		Serial.print(_val);
		Serial.print(", param1=");
		Serial.print(_param1);
		Serial.print(", param2=");
		Serial.println(_param2);
	}
	
	uint32_t c = HSVToRGB(_hue, _sat, _val, false);
	Serial.print("*** color = ");
	Serial.print(c >> 16, HEX);
	Serial.print(", ");
	Serial.print((c >> 8) & 0x0ff, HEX);
	Serial.print(", ");
	Serial.println(c & 0x0ff, HEX);
}
