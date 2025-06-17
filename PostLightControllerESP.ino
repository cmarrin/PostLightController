/*-------------------------------------------------------------------------
    This source file is a part of PostLightController
    For the latest info, see https://github.com/cmarrin/PostLightController
    Copyright (c) 2021-2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#if defined(ESP8266) || defined(ESP32)

/*

Post Light Controller for ESP

Each controller is an ESP32 SuperMini connected to an 8 pixel WS2812 ring on D10
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

   Command 	Name         	Params						Description
   -------  ----            ------    					-----------
	'C'     Constant Color 	color, n, d					Set lights to passed color
                                                        Flash n times, for d duration (in 100ms units)
                                                        If n == 0, just turn lights on

	'X'		EEPROM[0]		addr, <data>				Write EEPROM starting at addr. Data can be
														up to 64 bytes, due to buffering limitations.

			Rainbow			color, speed, range, mode	Change color through the rainbow by changing hue 
														from the passed color at the passed speed. Range
														is how far from the passed color to go, 0 (very little)
														to 7 (full range). If mode is 0 colors bounce 
														back and forth between start color through range. If
														it is 1 colors cycle the full color spectrum and
														range is ignored.

A Color param is 3 consecutive bytes (hue, saturation, value). Saturation and value are levels between 0 and 255.
Hue is an angle on the color wheel. A 0-360 degree value is obtained with hue / 256 * 360.

*/

#include <Adafruit_NeoPixel.h>

#include "Application.h"
#include "Flash.h"
#include "InterpretedEffect.h"

static constexpr int LEDPin = 10;
static constexpr int NumPixels = 8;
static constexpr const char* ConfigPortalName = "MT PostLightController";
static constexpr int32_t MaxDelay = 1000; // ms





//								default:
//								// See if it's interpreted, payload is after cmd bytes, offset it
//								if (!_interpretedEffect.init(_cmd, _buf + CommandSizeBefore, _payloadSize)) {
//									String errorMsg;
//									switch(_interpretedEffect.error()) {
//                                        case clvr::Memory::Error::None:
//                                        errorMsg = F("---");
//                                        break;
//                                        case clvr::Memory::Error::InvalidSignature:
//                                        errorMsg = F("bad signature");
//                                        break;
//                                        case clvr::Memory::Error::InvalidVersion:
//                                        errorMsg = F("bad  version");
//                                        break;
//                                        case clvr::Memory::Error::WrongAddressSize:
//                                        errorMsg = F("wrong addr size");
//                                        break;
//                                        case clvr::Memory::Error::NoEntryPoint:
//                                        errorMsg = F("no entry point");
//                                        break;
//                                        case clvr::Memory::Error::NotInstantiated:
//                                        errorMsg = F("not instantiated");
//                                        break;
//                                        case clvr::Memory::Error::UnexpectedOpInIf:
//                                        errorMsg = F("bad op in if");
//                                        break;
//                                        case clvr::Memory::Error::InvalidOp:
//                                        errorMsg = F("inv op");
//                                        break;
//                                        case clvr::Memory::Error::OnlyMemAddressesAllowed:
//                                        errorMsg = F("mem addrs only");
//                                        break;
//                                        case clvr::Memory::Error::AddressOutOfRange:
//                                        errorMsg = F("addr out of rng");
//                                        break;
//                                        case clvr::Memory::Error::ExpectedSetFrame:
//                                        errorMsg = F("SetFrame needed");
//                                        break;
//                                        case clvr::Memory::Error::InvalidModuleOp:
//                                        errorMsg = F("inv mod op");
//                                        break;
//                                        case clvr::Memory::Error::InvalidNativeFunction:
//                                        errorMsg = F("inv native func");
//                                        break;
//                                        case clvr::Memory::Error::NotEnoughArgs:
//                                        errorMsg = F("not enough args");
//                                        break;
//                                        case clvr::Memory::Error::WrongNumberOfArgs:
//                                        errorMsg = F("wrong arg cnt");
//                                        break;
//                                        case clvr::Memory::Error::StackOverrun:
//                                        errorMsg = F("can't call, stack full");
//                                        break;
//                                        case clvr::Memory::Error::StackUnderrun:
//                                        errorMsg = F("stack underrun");
//                                        break;
//                                        case clvr::Memory::Error::StackOutOfRange:
//                                        errorMsg = F("stack out of range");
//                                        break;
//                                        case clvr::Memory::Error::ImmedNotAllowedHere:
//                                        errorMsg = F("immed not allowed here");
//                                        break;
//                                        case clvr::Memory::Error::InternalError:
//                                        errorMsg = F("internal err");
//                                        break;
//									}
//
//									Serial.print(F("Interp fx err: "));
//									Serial.println(errorMsg);
//									showStatus(StatusColor::Red, 5, 1);
//									_state = State::NotCapturing;
//								} else {
//                                    _effect = Effect::Interp;
//								}
//								break;
//							}
//						}
//					}
//
//					break;
//			}




// This handles uris of the form /<root>/*
class HTTPPathHandler : public RequestHandler
{
  public:
    HTTPPathHandler(const String& root) : _root(root) { }
    
    virtual bool canHandle(WebServer&, HTTPMethod method, const String& uri) override
    {
        return uri.startsWith(_root.c_str());
    }
    
    virtual bool handle(WebServer& server, HTTPMethod method, const String& uri) override
    {
        if (method == HTTP_GET) {
            // Handle an operation like list
            CPString s = "Handled GET: filename='";
            s += uri;
            s += "', op='";
            s+= server.arg("op");
            s+= "'";
            cout << F("***** ") << s.c_str() << F("\n");
            server.send(200, "text/html", s.c_str());
            return true;
        }
        
        server.send(200);  // all done.
        return true;
    }

    virtual bool canUpload(WebServer&, const String &uri) override
    {
        return uri.startsWith(_root.c_str());
    }

    virtual void upload(WebServer& server, const String &uri, HTTPUpload &upload) override
    {
        CPString s = F("upload (filename='");
        s += upload.filename;
        s += F("' ");
        
        if (upload.status == UPLOAD_FILE_START) {
            s += F("START");
            _uploadSize = 0;
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            s += F("WRITE - received ");
            s += uint32_t(upload.currentSize);
            s += F(" bytes:");
            char buf[3] = "00";
            for (uint8_t i = 0; i < 10; ++i) {
                mil::toHex(upload.buf[i], buf);
                s += F(" 0x");
                s += buf;
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            s += F("END");
        }
        
        if (s.length() != 0) {
            cout << s << "\n";
            server.send(200, s.c_str());  // all done.
        }
    }
    
    virtual bool canRaw(WebServer&, const String &uri) override
    {
        cout << F("***** canRaw '") << uri.c_str() << F("'\n");
        return uri.startsWith(_root.c_str());
    }

    virtual void raw(WebServer& server, const String &uri, HTTPRaw &raw) override
    {
        cout << F("***** raw '") << uri.c_str() << F("'\n");
        CPString s = "During Raw: op='";
        s+= server.arg("op");
        s+= "'";
        cout << F("***** ") << s.c_str() << F("\n");
        
        if (raw.status == RAW_START) {
            //String fName = upload.filename;
            cout << F("****** raw started\n");

            // Open the file for writing
//            if (!fName.startsWith("/")) {
//                fName = "/" + fName;
//            }

    //            if (fsys->exists(fName)) {
    //                fsys->remove(fName);
    //            }
    //            _fsUploadFile = fsys->open(fName, "w");
            _uploadSize = 0;
        } else if (raw.status == RAW_WRITE) {
            cout << F("****** raw received ") << uint32_t(raw.currentSize) << F(" bytes\n");
            cout << F("****** bytes:");
            char buf[3] = "00";
            for (uint8_t i = 0; i < 10; ++i) {
                mil::toHex(raw.buf[i], buf);
                cout << " 0x" << buf;
            }
            cout << F("\n");
    //            if (_fsUploadFile) {
    //                size_t written = _fsUploadFile.write(upload.buf, upload.currentSize);
    //                
    //                if (written < upload.currentSize) {
    //                    // upload failed
    //                    cout << F("***** write error!\n");
    //                    _fsUploadFile.close();
    //
    //                    // delete file to free up space in filesystem
    //                    String fName = upload.filename;
    //                    if (!fName.startsWith("/")) {
    //                        fName = "/" + fName;
    //                    }
    //                    fsys->remove(fName);
    //                }
    //                
    //                _uploadSize += upload.currentSize;
    //            }
        } else if (raw.status == RAW_END) {
            cout << F("****** raw finished\n");
            server.send(200, "raw finished!!!");  // all done.
    //            if (_fsUploadFile) {
    //                _fsUploadFile.close();
    //            }
        }
    }

  private:
    String _root;
    size_t _uploadSize = 0;
};

class PostLightController : public mil::Application
{
public:
	PostLightController()
		: mil::Application(LED_BUILTIN, ConfigPortalName)
		, _pixels(NumPixels, LEDPin, NEO_GRB + NEO_KHZ800)
        , _pathHandler("/fs")
		, _interpretedEffect(&_pixels)
	{
    }

	~PostLightController() { }
	
	void setup()
	{
        delay(500);
        Application::setup();
        randomSeed(millis());

        setTitle("<center>MarrinTech PostLightController</center>");

        addHTTPHandler("/command", std::bind(&PostLightController::handleCommand, this));
        addCustomHTTPHandler(&_pathHandler);

	    _pixels.begin(); // This initializes the NeoPixel library.
	    _pixels.setBrightness(255);
		
        cout << F("Post Light Controller ESP v0.1\n");
      
		showStatus(StatusColor::Green, 3, 2);
	}

    void handleCommand()
    {
        cout << F("cmd='") << getHTTPArg("cmd") << F("'\n");
        sendHTTPPage("command processed");
    }

	void loop()
	{
        Application::loop();
        
		int32_t delayInMs = 0;
        if (_effect == Effect::Flash) {
            delayInMs = _flash.loop(&_pixels);
        } else if (_effect == Effect::Interp) {
            delayInMs = _interpretedEffect.loop();
        }
        
        if (delayInMs > MaxDelay) {
            delayInMs = MaxDelay;
        }
		
		if (delayInMs < 0) {
			// An effect has finished. End it and clear the display
			showColor(0, 0, 0, 0, 0);
            _effect = Effect::None;
			delayInMs = 0;
		}
		
		uint32_t newTime = millis();
		delay(delayInMs);
	}

private:
	enum class StatusColor { Red, Green, Yellow, Blue };

	void showColor(uint8_t h, uint8_t s, uint8_t v, uint8_t n, uint8_t d)
	{
        _effect = Effect::Flash;
        _flash.init(&_pixels, h, s, v, n, d);
	}
	
	void showStatus(StatusColor color, uint8_t numberOfBlinks = 0, uint8_t interval = 0)
	{
		// Flash half bright red or green at passed interval and passed number of times
        _effect = Effect::Flash;
        uint8_t h = 128;
        
        switch (color) {
            case StatusColor::Red: h = 0; break;
            case StatusColor::Green: h = 85; break;
            case StatusColor::Yellow: h = 30; break;
            case StatusColor::Blue: h = 140; break;
        }
        _flash.init(&_pixels, h, 0xff, 0x80, numberOfBlinks, interval);
	}
	
	Adafruit_NeoPixel _pixels;
 
    HTTPPathHandler _pathHandler;
	
    enum class Effect { None, Flash, Interp };
    Effect _effect = Effect::None;
	Flash _flash;
	InterpretedEffect _interpretedEffect;
	
	uint8_t _cmd = '0';
};

PostLightController controller;

void setup()
{
    Serial.begin(115200);
	controller.setup();
}

void loop()
{
	controller.loop();
}

#endif
