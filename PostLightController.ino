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

#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

#include "ConstantColor.h"
#include "Flicker.h"

constexpr int LEDPin = 6;
constexpr int NumPixels = 8;
constexpr int BufferSize = 10;
constexpr int ChecksumByte = BufferSize - 2;
constexpr char StartChar = '(';
constexpr char EndChar = ')';
constexpr unsigned long serialTimeOut = 2000; // ms

class PostLightController
{
public:
	PostLightController()
		: _pixels(NumPixels, LEDPin, NEO_GRB + NEO_KHZ800)
		, _serial(11, 10)
	{ }
	~PostLightController() { }
	
	void setup()
	{
	    Serial.begin(115200);
	    Serial.print("Hello\n");
	    _serial.begin(1200);

	    _pixels.begin(); // This initializes the NeoPixel library.
	    _pixels.setBrightness(255);
	
		_currentEffect = new ConstantColor(&_pixels);
		_currentEffect->init();
		_timeSinceLastChar = millis();
	}

	void loop()
	{
		uint32_t delayInMs = _currentEffect->loop();
		
	    if (_serial.available()) {
			// If it's been a while, reset the _capturing flag
			unsigned long newTime = millis();
			if (newTime - _timeSinceLastChar > serialTimeOut) {
				_capturing = false;
				Serial.print("***** resetting _capturing\n");
			}
			
			_timeSinceLastChar = newTime;
			
			char c = char(_serial.read());
			
			// Wait for a start char
			if (!_capturing) {
				if (c == StartChar) {
					_capturing = true;
					_bufIndex = 0;
				}
			}
			
			if (_capturing) {
				_buf[_bufIndex++] = c;
				if (_bufIndex >= BufferSize) {
					Serial.print("***** finished capturing\n");
					_buf[BufferSize] = '\0';
					
					// We have a bufferful
					if (_buf[BufferSize - 1] != EndChar) {
						Serial.print("MALFORMED BUFFER: unexpected end char=");
						Serial.print(_buf[BufferSize - 1]);
						Serial.print(", cmd: ");
						Serial.println(_buf);
					}
					
					// First make sure checksum is right
					uint8_t expectedChecksum = _buf[ChecksumByte];
					_buf[ChecksumByte] = '0';
					uint8_t actualChecksum = checksum(_buf, BufferSize) + 0x30;
					if (expectedChecksum != actualChecksum) {
						Serial.print("CRC ERROR: expected=");
						Serial.print(expectedChecksum);
						Serial.print(", actual=");
						Serial.print(actualChecksum);
						Serial.print(", cmd: ");
						Serial.println(_buf);
					} else {
						// Process command
						Serial.print("Processing cmd: ");
						Serial.println(_buf);
						
						if (_currentEffect) {
							delete _currentEffect;
							_currentEffect = nullptr;
						}
						
						switch(_buf[2]) {
							case 'C':
							_currentEffect = new ConstantColor(&_pixels);
							break;
							
							case 'F':
							_currentEffect = new Flicker(&_pixels);
							break;
						}
						
						if (_currentEffect) {
							_currentEffect->init(_buf + 3, BufferSize - 5);
						}
					}
					
					_capturing = false;
				}
			}
	  	}

		// Eventually we can't delay in loop because we have to feed the soft serial port
		// But for now...
		delay(delayInMs);
	}

private:
	static uint8_t checksum(const char* str, int length) 
	{
	    uint8_t sum = 0;

	    for (int i = 0; i < length; i++) {
			sum += str[i];
	        uint8_t extract = str[i];
	    }
	    return sum & 0x3f;
	}

	SoftwareSerial _serial;
	Adafruit_NeoPixel _pixels;
	
	Effect* _currentEffect = nullptr;
	
	char _buf[BufferSize + 1];
	char _bufIndex = 0;
	bool _capturing = false;
	unsigned long _timeSinceLastChar = 0;
};

PostLightController controller;

void setup()
{
	controller.setup();
}

void loop()
{
	controller.loop();
}
