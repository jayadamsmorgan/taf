local taf = require("taf")

local target = taf.get_current_target()
local custom_lib = require("my-custom-lib")
local variables = require("variables." .. target)

taf.test("Testing multitarget project", function()
	local baudrate = variables.serial.baudrate
	print("Serial baudrate for target '" .. taf.get_current_target() .. "' is " .. baudrate)
	if target == "target1" then
		assert(baudrate == 115200)
	elseif target == "target2" then
		assert(baudrate == 1500000)
	end

	local result = custom_lib.keyword2("this is ", "test ")
	print(result)
	if target == "target1" then
		assert(result == "this is test ")
	elseif target == "target2" then
		assert(result == "this is test hello")
	end
end)
