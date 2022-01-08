// Copyright Chris Marrin 2021
//
// Effect Base Class

#include "Effect.h"


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
