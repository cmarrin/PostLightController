-- Lua script for Flicker Post Light Controller effect
--
-- Args are sent in as an array to the setup() function:
--
--		0, 1, 2		- Color (hsv) 0 to 255. Clamped by hsvToRGB() if out of range
--		3			- Speed 0 (slow) to 7 (fast). Values out of range are clamped.
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

local Effect = { }

Effect.Delay = 30 -- Delay between iterations (in ms)

Effect.FlickerBrightnessMin = 77
Effect.FlickerBrightnessMaxMin = 120 -- brightnessMax has to be this much more than FlickerBrightnessMin so we get flicker

Effect.IncMinMaxDiff = 1.2
Effect.IncConstant = 400 -- value multiplied by speed and then inverted to get base increment (lower numbers are faster)

Effect.hue = 0
Effect.sat = 0
Effect.val = 0
Effect.speed = 0

Effect.brightnessMax = 0
Effect.incMin = 0
Effect.incMax = 0

function Effect.clearArray(array, size)
	for i = 1, size, 1 do
		array[i] = 0;
	end
end

Effect.ledCur = { }
Effect.clearArray(Effect.ledCur, NumPixels)
Effect.ledInc = { }
Effect.clearArray(Effect.ledInc, NumPixels)
Effect.ledMax = { }
Effect.clearArray(Effect.ledMax, NumPixels)

-- arg is a table containing key/value pairs
function Effect.setArgs(arg)
	-- Get args from the table
	for key, value in pairs(arg) do
		if key == "color" then
			Effect.hue, Effect.sat, Effect.val = htmlToHSV(value)
		elseif key == "speed" then
			Effect.speed = tonumber(value)
		end
	end
	
	-- Range limit and set internal values
	if Effect.speed < 0 then
		Effect.speed = 0
	end

	if Effect.speed > 7 then
		Effect.speed = 7
	end

	-- val gets scaled  so it is 0 to 255 and maxBrightness = FlickerBrightnessMaxMin to 255
	Effect.brightnessMax = math.max(0, math.min(255, Effect.val))
	Effect.brightnessMax = Effect.brightnessMax / 255 * (255 - Effect.FlickerBrightnessMaxMin) + Effect.FlickerBrightnessMaxMin

	Effect.incMin = 1 / ((8 - Effect.speed) * Effect.IncConstant / Effect.Delay)
	Effect.incMax = Effect.incMin * Effect.IncMinMaxDiff
end

function Effect.animate(index)
    --  Watch for overflow
    if Effect.ledInc[index] > 0 then
        if Effect.ledCur[index] >= Effect.ledMax[index] - Effect.ledInc[index] then
            Effect.ledInc[index] = -Effect.ledInc[index]
            Effect.ledCur[index] = Effect.ledMax[index]
            return 1
        end
    elseif Effect.ledCur[index] <= Effect.ledInc[index] then
		Effect.ledInc[index] = -Effect.ledInc[index]
		Effect.ledCur[index] = 0
		return -1
	end

    Effect.ledCur[index] = Effect.ledCur[index] + Effect.ledInc[index]
    return 0
end

function Effect.loop()
	local loopStartTime = millis();
	for i = 1, NumPixels, 1 do
		if Effect.animate(i) == -1 then
			-- We are done with the throb. We always start at 0
			Effect.ledCur[i] = 0
			
			-- Select a new inc (how fast it pulses), and  max (how bright it
			-- gets) by setting the inc to a random value between incMin and incMax
			Effect.ledInc[i] = math.random() * (Effect.incMax - Effect.incMin) + Effect.incMin
			
			-- set the max brightness to a random value
			Effect.ledMax[i] = math.random()
		end
		
		-- ledCur is a value between 0 (FlickerBrightestMin) and 1 (brightnessMax)
		local brightness = Effect.ledCur[i] * (Effect.brightnessMax - Effect.FlickerBrightnessMin) + Effect.FlickerBrightnessMin
		setLED(1, i - 1, hsvToRGB(Effect.hue, Effect.sat, brightness))
	end
	refreshLEDs(1)
	
	local delayTime = Effect.Delay - (millis() - loopStartTime)
	if delayTime < 1 then
		delayTime = 1
	end

	return delayTime
end

return Effect
