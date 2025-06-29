-- The following structure is just an example,
-- it could be whatever you want as long as it remains
-- the same in structure between all other variables/some_target.lua

local M = {
	serial = {
		baudrate = 1500000,
		parity = "odd",
	},
	web = {
		host = "localhost",
		port = 9515,
	},
}

return M
