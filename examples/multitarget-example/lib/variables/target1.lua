-- The following structure is just an example,
-- it could be whatever you want as long as it remains
-- the same in structure between all other variables/some_target.lua

local M = {
	serial = {
		baudrate = 115200,
		parity = "none",
	},
	web = {
		host = "localhost",
		port = 9515,
	},
}

return M
