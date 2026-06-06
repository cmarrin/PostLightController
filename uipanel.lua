-- Turn lights on and off and set color

__print__("Lua UI Panel program running\n")

PixelsPerPost = 8
NumPosts = 7
NumPixels = PixelsPerPost * NumPosts

setLEDs(1, 0, NumPixels, 0, 0, 0)
refreshLEDs(1)

local on = false
local effect = nil
local effectName = ""

function showMemoryUsage(s)
	local before = collectgarbage("count")
	collectgarbage()
	local after = collectgarbage("count")
	__print__(string.format("%s: Mem used before gc=%f, after=%f\n", s, before, after))
end

function terminateEffect()
	showMemoryUsage("Before terminateEffect")
	setLEDs(1, 0, NumPixels, 0, 0, 0)
	refreshLEDs(1)
	package.loaded[effectName] = nil
	effect = nil
	showMemoryUsage("After terminateEffect")
end

-- Wait for events
while true do
	local ev = getEvent()
	if ev then
		for key, value in pairs(ev) do
			if key == "effect" then
				if effect then
					-- terminate the old effect
					terminateEffect()
				end
				
				effectName = value
			elseif key == "on" then
				-- Turn the effect on or off
				on = value == "true"
			elseif key == "color" then
				-- set color
				__print__(string.format("**** uipanel.lua:color='%s'\n", value))
			else
				__print__(string.format("**** uipanel.lua:unknown event, kay='%s', value='%s'\n", key, value))
			end
		end
	end
	
	-- run the effect if it's on
	if effectName ~= "" then
		if on then
			if not effect then
				__print__(string.format("**** running effect '%s'\n", effectName))
				effect = require(effectName)
				effect.setup({ 30, 255, 128, 2 }) -- Dummy setup
			end
			delay(effect.loop())
		else
			-- Effect has been turned off
			if effect then
				__print__("**** effect is off, terminating\n")
				terminateEffect()
			end
		end
	else
		delay(10)
	end
end
