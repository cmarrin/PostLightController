// Copyright Chris Marrin 2021

/*

Pond Light Controller

Each controller is a 5V Arduino Nano with serial data coming in on pin D5 and data out to
an 8 pixel WS2812 ring on D6. Serial data, +5V and Gnd comes into each post and goes out to
the next post.

Serial Protocol

Standard RS232 protocol is used with 1 start bit, 8 data bits and 1 stop bit. Bit rate is
set by the protocol from 1200 bps to 57600 bps. Bit rate is currently fixed at 1200 baud.

Each command has the format:

	Lead-in		'('
	Address		Which controller this command is for (0 is all devices)
	Command		Single char command 
	Size		Payload size in bytes
	Payload		Bytes of payload (in binary)
	Checksum	One byte checksum
	Lead-out	')'

All characters but the payload and payload size are between 0x30 and 0x6f. Max payload size is
256 bytes. Checksum is computed by adding all characters of the buffer, including leading and 
ending characters, truncating the result to 6 bits and adding 0x30 to the result. For purposes 
of computing checksum, a '0' is placed in the checksum location.

To allow for interpreted commands each "normal" command (those which cause the lights to do a 
pattern) are limited to 16 bytes. This is enough for 4 colors (12 bytes) plus 4 byte params. The only
command which can exceed this is the 'X' command.

Command List:

   Command 	Name         	Params						Description
   -------  ----            ------    					-----------
	'C'     Constant Color 	color						Set lights to passed color

	'F'		Flicker			color, speed				Flicker lights based on passed color at 
														passed speed (0-7).

	'X'		EEPROM[0]		addr, <data>				Write EEPROM starting at addr. Data can be
														up to 64 bytes, due to buffering limitations.

Proposed commands for later

			Throb			color, speed, depth			Throb lights with passed color, dimming at passed
														speed (0-7). Passed depth is amount of dimming
														0 (slight dimming) to 7 (dimming to 0 brightness)

			Rainbow			color, speed, range, mode	Change color through the rainbow by changing hue 
														from the passed color at the passed speed. Range
														is how far from the passed color to go, 0 (very little)
														to 7 (full range). If mode (1 bit) is 0 colors bounce 
														back and forth between start color through range. If
														it is 1 colors cycle the full color spectrum and
														range is ignored.

A Color param is 3 consecutive bytes (hue, saturation, value). Saturation and value are levels between 0 and 255.
Hue is an angle on the color wheel. A 0-360 degree value is obtained with hue / 256 * 360.

*/

#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#include "ConstantColor.h"
#include "Flicker.h"
#include "Flash.h"
#include "Interpreter.h"

constexpr int LEDPin = 6;
constexpr int NumPixels = 8;
constexpr int BufferSize = 65;
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
		if (_state != State::NotCapturing) {
			if (newTime - _timeSinceLastChar > SerialTimeOut) {
				_state = State::NotCapturing;
				Serial.print("***** Too Long since last char, resetting\n");
				showStatus(StatusColor::Red, 3, 2);
			}
			_state = State::NotCapturing;
		}

	    if (_serial.available()) {
			_timeSinceLastChar = newTime;
			
			char c = char(_serial.read());
			
			// Run the state machine
			switch(_state) {
				case State::NotCapturing:
					if (c == StartChar) {
						_state = State::DeviceAddr;
						_expectedChecksum = c;
					}
					break;
				case State::DeviceAddr:
					if (c != '0') {
						// FIXME: Handle commands for other devices
					}
					_state = State::Cmd;
					_expectedChecksum += c;
					break;
				case State::Cmd:
					_cmd = c;
					_state = State::Size;
					_expectedChecksum += c;
					break;
				case State::Size:
					_bufSize = c - '0';
					if (_bufSize > BufferSize) {
						Serial.print("Buffer size too big. Size=");
						Serial.println(_bufSize);
						showStatus(StatusColor::Red, 6, 1);
						_state = State::NotCapturing;
					} else {
						_state = State::Data;
						_bufIndex = 0;
						_expectedChecksum += c;
					}
					break;
				case State::Data:
					_buf[_bufIndex++] = c;
					_expectedChecksum += c;
					
					if (_bufIndex >= _bufSize) {
						_state = State::Checksum;
					}
					break;
				case State::Checksum: {
					_state = State::LeadOut;
					_actualChecksum = c;
					_expectedChecksum += '0';
					break;
				}
				case State::LeadOut:
					if (c != EndChar) {
						Serial.println("Expected lead-out char");
						showStatus(StatusColor::Red, 6, 1);
						_state = State::NotCapturing;
					} else {
						// Make sure checksum is right
						_actualChecksum = (_actualChecksum & 0x3f) + 0x30;
						
						if (_expectedChecksum != _actualChecksum) {
							Serial.print("CRC ERROR: expected=");
							Serial.print(_expectedChecksum);
							Serial.print(", actual=");
							Serial.print(_actualChecksum);
							Serial.print(", cmd: ");
							Serial.println(_cmd);
							showStatus(StatusColor::Red, 5, 5);
							_state = State::NotCapturing;
						} else {
							// Have a good buffer, handle the command
							_state = State::NotCapturing;
							
							switch(_cmd) {
								case 'C':
								_currentEffect = &_constantColorEffect;
								break;
								
								case 'F':
								_currentEffect = &_flickerEffect;
								break;
								
								case 'X':
								if (_buf[0] + _bufSize - 1 > 1024) {
									Serial.print("EEPROM buffer out of range: addr=");
									Serial.print(_buf[0]);
									Serial.print(", size=");
									Serial.println(_bufSize - 1);
									showStatus(StatusColor::Red, 5, 5);
									_state = State::NotCapturing;
								} else {
									uint8_t startAddr = _buf[0];
									for (uint8_t i = 0; i < _bufSize - 1; ++i) {
										EEPROM[i + startAddr] = _buf[i + 1];
									}
								}
								break;
								
								default:
								Serial.print("Unrecognized command: ");
								Serial.println(_buf[2], HEX);
								showStatus(StatusColor::Red, 8, 4);
								_state = State::NotCapturing;
								break;
							}

							if (_currentEffect) {
								_currentEffect->init(_buf, _bufSize);
							}
						}
					}

					break;
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
	
	uint8_t _buf[BufferSize];
	uint8_t _bufIndex = 0;
	uint8_t _bufSize = 0;
	unsigned long _timeSinceLastChar = 0;
	
	uint8_t _expectedChecksum = 0;
	uint8_t _actualChecksum = 0;
	
	uint8_t _cmd = '0';
	
	Device _device;
	
	// State machine
	enum class State { NotCapturing, DeviceAddr, Cmd, Size, Data, Checksum, LeadOut };
	State _state = State::NotCapturing;
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
