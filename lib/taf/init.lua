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
--- @param tags_or_body [string]|function either array of test tags or test body
--- @param body function|nil body of the test if previous argument is tags
M.test = function(test_name, tags_or_body, body)
	tm:test(test_name, tags_or_body, body)
end

--- @param defer_func function function executed on defer
--- @param ... any arguments to pass to the function
M.defer = function(defer_func, ...)
	tm:defer(defer_func, ...)
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
