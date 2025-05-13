local serial = require("serial")
local vars = require("variables")

local serial_timeout = 1000

local M = {}

M.start_serial_device = function()
	serial.start(vars.device_port, vars.bits, vars.parity)
end

M.check_for_expected = function(result, expected)
	return string.find(result, expected)
end

M.print_pwd = function()
	serial.write("pwd")
	local result = serial.read_until(serial_timeout)
end

return M
