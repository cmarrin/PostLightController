// Copyright Chris Marrin 2021
//
// Effect Base Class
//
// This class is the base class for all effects

#pragma once

#include <Adafruit_NeoPixel.h>

class Color
{
public:
	Color() { }
	Color(uint8_t h, uint8_t s, uint8_t v, bool gammaCorrect = true)
	{
		setColor(h, s, v, gammaCorrect);
	}
	
	Color(float h, float s, float v, bool gammaCorrect = true)
	{
		setColor(min(round(h * 255), 255),
				 min(round(s * 255), 255),
				 min(round(v * 255), 255),
				 gammaCorrect);
	}
	
	uint32_t rgb() const
	{
		uint16_t h = min(max(_hue * 65535, 0), 65535);
		uint8_t s = min(max(_sat * 255, 0), 255);
		uint8_t v = min(max(_val * 255, 0), 255);
		uint32_t c = Adafruit_NeoPixel::ColorHSV(h, s, v);
		return _gammaCorrect ? Adafruit_NeoPixel::gamma32(c) : c;
	}
	
	float hue() const { return _hue; }
	float sat() const { return _sat; }
	float val() const { return _val; }
	
private:
	void setColor(uint8_t a, uint8_t b, uint8_t c, bool gammaCorrect = true);
	
	float _hue = 0;
	float _sat = 0;
	float _val = 0;
	bool _gammaCorrect = true;
};

class Effect
{
public:
	Effect(Adafruit_NeoPixel* pixels) : _pixels(pixels) { }
	
	virtual ~Effect() { }

	virtual void init(const uint8_t* = nullptr, uint32_t = 0) { }
	
	// Return delay in ms
	virtual int32_t loop() = 0;
		
protected:
	static uint8_t cmdParamToValue(uint8_t param) { return (param - 0x30) & 0x3f; }
	
	Adafruit_NeoPixel* _pixels = nullptr;
	const uint8_t* _buf = nullptr;
	uint32_t _size = 0;
	
private:
};
