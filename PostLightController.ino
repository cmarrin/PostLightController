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
#include "Flash.h"
#include "Interpreter.h"

constexpr int LEDPin = 6;
constexpr int NumPixels = 8;
constexpr int BufferSize = 10;
constexpr int ChecksumByte = BufferSize - 2;
constexpr char StartChar = '(';
constexpr char EndChar = ')';
constexpr unsigned long SerialTimeOut = 2000; // ms

class Device : public arly::Interpreter
{
public:
    virtual uint8_t rom(uint16_t i) const override
    {
		(void) i;
        return 0;
    }
    
    virtual void setLight(uint8_t i, uint32_t rgb) override
    {
        // FIXME;
		(void) i;
		(void) rgb;
    }
    
    virtual uint8_t numPixels() const override
    {
        return NumPixels;
    }

    void logAddr(uint16_t addr) const
	{
		Serial.print("[");
		Serial.print(addr);
		Serial.print("]");
	}
    
    virtual void log(uint16_t addr, uint8_t r, int32_t v) const override
    {
        logAddr(addr);
		Serial.print(": r[");
		Serial.print(r);
		Serial.print("] = ");
		Serial.println(v);
    }
    virtual void logFloat(uint16_t addr, uint8_t r, float v) const override
    {
        logAddr(addr);
		Serial.print(": r[");
		Serial.print(r);
		Serial.print("] = ");
		Serial.println(v);
    }

    virtual void logColor(uint16_t addr, uint8_t r, const Color& c) const override
    {
        logAddr(addr);
        Serial.print(": c[");
		Serial.print(r);
		Serial.print("] = (");
		Serial.print(c.hue());
		Serial.print(", ");
		Serial.print(c.sat());
		Serial.print(", ");
		Serial.print(c.val());
		Serial.println(")");
    }

private:
};

class PostLightController
{
public:
	PostLightController()
		: _pixels(NumPixels, LEDPin, NEO_GRB + NEO_KHZ800)
		, _serial(11, 10)
		, _constantColorEffect(&_pixels)
		, _flickerEffect(&_pixels)
		, _flashEffect(&_pixels)
	{ }
	~PostLightController() { }
	
	void setup()
	{
	    Serial.begin(115200);
	    _serial.begin(1200);
		
		delay(500);

	    _pixels.begin(); // This initializes the NeoPixel library.
	    _pixels.setBrightness(255);
		
		Serial.println("Post Light Controller v0.1");
	
		showStatus(StatusColor::Green, 3, 2);
		
		_timeSinceLastChar = millis();
	}

	void loop()
	{
		_device.loop();
		
		int32_t delayInMs = _currentEffect ? _currentEffect->loop() : 0;
		
		if (delayInMs < 0) {
			// An effect has finished. End it and clear the display
			_currentEffect = &_constantColorEffect;
			_currentEffect->init();
			delayInMs = 0;
		}
		
		uint32_t newTime = millis();

		// If we're capturing and it's been a while, error
		if (_capturing) {
			if (newTime - _timeSinceLastChar > SerialTimeOut) {
				_capturing = false;
				Serial.print("***** Too Long since last char, resetting\n");
				showStatus(StatusColor::Red, 3, 2);
			}
		}

	    if (_serial.available()) {
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
					
					delayInMs = 0;

					// We have a bufferful
					if (_buf[BufferSize - 1] != EndChar) {
						Serial.print("MALFORMED BUFFER: unexpected end char=");
						Serial.print(_buf[BufferSize - 1]);
						Serial.print(", cmd: ");
						Serial.println(_buf);
						showStatus(StatusColor::Red, 6, 1);
					} else {
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
							showStatus(StatusColor::Red, 5, 5);
						} else {
							// Process command
							Serial.print("Processing cmd: [ ");
							for (int i = 0; i < BufferSize; ++i) {
								Serial.print(_buf[i], HEX);
								Serial.print(" ");
							}
							Serial.println("]");
						
							switch(_buf[2]) {
								case 'C':
								_currentEffect = &_constantColorEffect;
								break;
							
								case 'F':
								_currentEffect = &_flickerEffect;
								break;
								default:
								Serial.print("Unrecognized command: ");
								Serial.println(_buf[2], HEX);
								showStatus(StatusColor::Red, 8, 4);
								break;
							}
						
							if (_currentEffect) {
								_currentEffect->init(reinterpret_cast<uint8_t*>(_buf + 3), BufferSize - 5);
							}
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
	static uint8_t checksum(const char* buf, int length) 
	{
	    uint8_t sum = 0;

	    for (int i = 0; i < length; i++) {
			sum += buf[i];
	    }
	    return sum & 0x3f;
	}

	enum class StatusColor { Red, Green, Yellow };
	
	void showStatus(StatusColor color, uint8_t numberOfBlinks, uint8_t interval)
	{
		// Flash full bright red at 1 second interval, 10 times
		uint8_t buf[ ] = { 0x00, 0xff, 0xff, numberOfBlinks, interval };
		buf[0] = (color == StatusColor::Red) ? 0 : ((color == StatusColor::Green) ? 85 : 42);
		_currentEffect = &_flashEffect;
		_currentEffect->init(buf, sizeof(buf));		
	}
	
	Adafruit_NeoPixel _pixels;
	SoftwareSerial _serial;
	
	Effect* _currentEffect = nullptr;
	
	ConstantColor _constantColorEffect;
	Flicker _flickerEffect;
	Flash _flashEffect;
	
	char _buf[BufferSize + 1];
	int _bufIndex = 0;
	bool _capturing = false;
	unsigned long _timeSinceLastChar = 0;
	
	Device _device;
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
