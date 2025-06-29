local taf = require("taf")

local variables = require("variables." .. taf.get_current_target())

taf.test("Testing multitarget project", function()
	print("Serial baudrate for this target '" .. taf.get_current_target() .. "' is " .. variables.serial.baudrate)
end)
