--
-- testLED - test the operation of the led_strip API
--

-- Test the round 8 LED
initLED(1, 10, 8)

local down = false

for j = 1, 4, 1 do
	for i = 0, 200, 4 do
		local brightness = down and (200 - i) or i
		setLED(1, 0, 0, 0, brightness)
		setLED(1, 1, 0, brightness, 0)
		setLED(1, 2, 0, brightness, brightness)
		setLED(1, 3, brightness, 0, 0)
		setLED(1, 4, brightness, 0, brightness)
		setLED(1, 5, brightness, brightness, 0)
		setLED(1, 6, brightness, brightness, brightness)
		setLED(1, 7, brightness, 0, 0)
		refreshLEDs(1)
		delay(20)
	end
	down = not down
end

__print__("LEDTest finished\n")

--[[
-- Test the 60 LED strip
initLED(1, 11, 60)

local down = false

-- Pulse
for j = 1, 4, 1 do
	for i = 0, 50, 1 do
		local brightness = down and (50 - i) or i

		for b = 0, 59, 1 do
			setLED(1, b, 0, 0, brightness)
		end
		refreshLEDs(1)
		delay(20)
	end
	down = not down
end

-- Race
for j = 1, 4, 1 do
	for led = 0, 62, 1 do
		local i = down and led or (62 - led)
		for b = 0, 59, 1 do
			if b < i - 2  or b > i + 2 then
				setLED(1, b, 0, 0, 0)
			elseif b == i - 2 or b == i + 2 then
				setLED(1, b, 0, 10, 0)
			elseif b == i - 1 or b == i + 1 then
				setLED(1, b, 0, 30, 0)
			else 
				setLED(1, i, 0, 50, 0)
			end
		end
		refreshLEDs(1)
		delay(20)
	end
	down = not down
end
--]]
