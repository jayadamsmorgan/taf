local tm = require("taf-main")

local M = {}

-- Expose submodules of 'taf'
M.serial = require("taf.serial")
M.web = require("taf.web")

--- Get amount of milliseconds since test started
---
--- @return number ms
M.millis = function()
	return tm:millis()
end

--- Register new test
---
--- @param test_name string name of the test
--- @param body function
M.test = function(test_name, body)
	tm:test(test_name, body)
end

--- Print something to logs & TUI. Same as default Lua `print()`
---
M.print = function(...)
	tm:print(...)
end

--- Put test to sleep for `ms` amount of milliseconds
---
--- @param ms number
M.sleep = function(ms)
	tm:sleep(ms)
end

return M
