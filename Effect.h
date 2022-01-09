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
	enum class Model { HSV, RGB };
	
	Color() { }
	Color(uint8_t a, uint8_t b, uint8_t c, Model model = Model::RGB, bool gammaCorrect = true)
	{
		setColor(a, b, c, model, gammaCorrect);
	}
	
	Color(float h, float s, float v, bool gammaCorrect = true)
	{
		setColor(min(round(h * 255), 255),
				 min(round(s * 255), 255),
				 min(round(v * 255), 255),
				 Model::HSV, gammaCorrect);
	}
	
	uint32_t rgb() const { return _color; }
	void hsv(float& h, float& s,float& v) const;

private:
	void setColor(uint8_t a, uint8_t b, uint8_t c, Model model = Model::RGB, bool gammaCorrect = true);
	
	uint32_t _color = 0;
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
