-- Lua script for Flicker Post Light Controller effect
--
-- Command:
--
--      'f' - Flicker: Single color flickers randomly at passed speed
--              Args:   0, 1, 2     Color
--                      3           Speed (0-7)
-- Flicker effect
--
-- Args:    0, 1, 2     Color
--          3           Speed (0-7)
--
-- Flicker inc value. This is a random value based on speed.
-- Speed goes from 0-7 for flicker. We use floating point
-- values from 0 to 1. It's 30ms per animation step. A slow 
-- flicker is over 5 seconds so for speed 0 the inc should be
-- 0.006. A fast flicker is 8 times faster, so 0.625 seconds, 
-- so for speed 7 the inc should be 0.05. We add some randomness 
-- of around 10%, so the inc values will be determined by:
--
--		incMin = 1 / ((8 - speed) * 625 / 30)
--		incMax = incMin * 1.1
--
-- The current brightness value (ledCur) is the current animated
-- value, from 0 to 1. We scale that from FlickerBrightnessMin,
-- a value chosen to prevent the LEDs from strobing, to the
-- passed in 'v' value.
-- 
-- Flicker needs all LedEntries because all leds on all posts flicker independently.
-- So the first PixelsPerPost entries are for the first post, the next entries
-- are for the second post and so on.

local TestTiming = false

local PixelsPerPost = 8
local NumPosts = 7
local NumPixels = PixelsPerPost * NumPosts
local Delay = 30 -- Delay between iterations (in ms)

local FlickerBrightnessMin = 77
local FlickerBrightnessMaxMin = 120 -- brightnessMax has to be this much more than FlickerBrightnessMin so we get flicker

local IncMinMaxDiff = 1.2
local IncConstant = 400 -- value multiplied by speed and then inverted to get base increment (lower numbers are faster)

function clearArray(array, size)
	for i = 1, size, 1 do
		array[i] = 0;
	end
end

-- Get the params
local h = tonumber(arg[1])
local s = tonumber(arg[2])
local v = tonumber(arg[3])
local speed = tonumber(arg[4])

if speed < 0 then
	speed = 0
end

if speed > 7 then
	speed = 7
end

-- v gets scaled  so v = 0 to 255 and maxBrightness = FlickerBrightnessMaxMin to 255
local brightnessMax = math.max(0, math.min(255, v))
brightnessMax = brightnessMax / 255 * (255 - FlickerBrightnessMaxMin) + FlickerBrightnessMaxMin

local incMin = 1 / ((8 - speed) * IncConstant / Delay)
local incMax = incMin * IncMinMaxDiff

local ledCur = { }
clearArray(ledCur, NumPixels)
local ledInc = { }
clearArray(ledInc, NumPixels)
local ledMax = { }
clearArray(ledMax, NumPixels)

function animate(index)
    --  Watch for overflow
    if ledInc[index] > 0 then
        if ledCur[index] >= ledMax[index] - ledInc[index] then
            ledInc[index] = -ledInc[index]
            ledCur[index] = ledMax[index]
            return 1
        end
    elseif ledCur[index] <= ledInc[index] then
		ledInc[index] = -ledInc[index]
		ledCur[index] = 0
		return -1
	end

    ledCur[index] = ledCur[index] + ledInc[index]
    return 0
end

if TestTiming then
	t = millis()
	timeCount = 100
end

while true do
	local loopStartTime = millis();
	for i = 1, NumPixels, 1 do
		if animate(i) == -1 then
			-- We are done with the throb. We always start at 0
			ledCur[i] = 0
			
			-- Select a new inc (how fast it pulses), and  max (how bright it
			-- gets) by setting the inc to a random value between incMin and incMax
			ledInc[i] = math.random() * (incMax - incMin) + incMin
			
			-- set the max brightness to a random value
			ledMax[i] = math.random()
		end
		
		-- ledCur is a value between 0 (FlickerBrightestMin) and 1 (brightnessMax)
		local brightness = ledCur[i] * (brightnessMax - FlickerBrightnessMin) + FlickerBrightnessMin
		setLED(1, i - 1, hsvToRGB(h, s, brightness))
	end
	refreshLEDs(1)
	
	local delayTime = Delay - (millis() - loopStartTime)
	if delayTime < 1 then
		delayTime = 1
	end
	delay(delayTime)

	if TestTiming then
		timeCount = timeCount - 1
		if timeCount == 0 then
			local t2 = millis()
			local diff = t2 - t
			t = t2
			timeCount = 100
			__print__("difftime is "..(diff / 100).."\n")
		end
	end
end
