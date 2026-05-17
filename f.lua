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
-- Speed goes from 0-7 for flicker. Min and max values are
-- from 0 to 255 multiplied by 128. so a typical range is
-- 15000. If we want to animate 15000 values in 1 second
-- at 25ms per cycle, the inc value would be 375. A slow
-- animation would be around 5 seconds (inc = 2000). A fast
-- animation would be around 1/2 second (inc = 200).
-- These table entries are /128 (so they fit in a byte)
-- so they should go from a slow of 1 to a fast of 16

-- Animation values are 16 bit integers, representing a fixed point
-- number, with 9 bits representing the signed integer part and 7 
-- bits for the fraction. So numbers can be +/- 255 with 2 digits 
-- of decimal precision. To get a brightness value from 'cur' you 
-- simply shift right 7 places or divide by 128.

-- Flicker need all LedEntries because all leds on all posts flicker independently.
-- So the first PixelsPerPost entries are for the first post, the next entries
-- are for the second post and so on.

local PixelsPerPost = 8
local NumPosts = 7
local NumPixels = PixelsPerPost * NumPosts
local Delay = 30; -- Delay between iterations (in ms)

local FlickerMin = 38
local FlickerBrightestMin = 77;

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

local brightnessMax = math.max(FlickerBrightestMin, math.min(255, v))

local speedMin = speed + 1
local speedMax = speed + 2

local ledCur = { }
clearArray(ledCur, NumPixels)
local ledInc = { }
clearArray(ledInc, NumPixels)
local ledMin = { }
clearArray(ledMin, NumPixels)
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
    elseif ledCur[index] <= ledMin[index] - ledInc[index] then
		ledInc[index] = -ledInc[index]
		ledCur[index] = ledMin[index]
		return -1
	end

    ledCur[index] = ledCur[index] + ledInc[index]
    return 0
end

while true do
	local loopStartTime = millis();
	for i = 1, NumPixels, 1 do
		if animate(i) == -1 then
			-- We are done with the throb. We always start at BrightnessMin.
			-- Select a new inc (how fast it pulses), and  max (how bright it
			-- gets) based on the speed value.
			ledCur[i] = FlickerMin * 128
			ledMin[i] = ledCur[i]
			
			-- Set the inc to a random value from the table
			ledInc[i] = math.random(speedMin, speedMax) * 128
			
			-- set the max brightness for flicker
			ledMax[i] = math.random(FlickerBrightestMin, brightnessMax) * 128
		end

		setLED(1, i - 1, hsvToRGB(h, s, ledCur[i] / 128))
	end
	refreshLEDs(1)
	
	local delayTime = Delay - (millis() - loopStartTime)
	if delayTime < 1 then
		delayTime = 1
	end
	delay(delayTime)
end
