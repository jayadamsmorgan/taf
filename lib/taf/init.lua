local tm = require("taf-main")

--- @alias log_level
--- | '"c"' critical
--- | '"C"' critical
--- | '"critical"'
--- | '"CRITICAL"'
--- | '"e"' error
--- | '"E"' error
--- | '"error"'
--- | '"ERROR"'
--- | '"w"' warning
--- | '"W"' warning
--- | '"warning"'
--- | '"WARNING'
--- | '"i"' info
--- | '"I"' info
--- | '"info"'
--- | '"INFO"'
--- | '"d"' debug
--- | '"D"' debug
--- | '"debug"'
--- | '"DEBUG"'
--- | '"t"' trace
--- | '"T"' trace
--- | '"trace"'
--- | '"TRACE"'

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

--- Print something to logs & TUI. Same as default Lua `print()`. Both will use 'info' log level
---
--- @param ... any
M.print = function(...)
	tm:print(...)
end

--- Print something to logs & TUI with specified log level.
--- If `log_level` is "critical" - fails test immedeately
--- If `log_level` is "error" - fails test but continues to execute
--- Other `log_level` will just log
---
--- @param log_level log_level
--- @param ... any
M.log = function(log_level, ...)
	tm:log(log_level, ...)
end

--- Print something to logs & TUI with 'critical' log level.
--- Will fail test immedeately
---
--- @param ... any
M.log_critical = function(...)
	tm:log("c", ...)
end

--- Print something to logs & TUI with 'error' log level.
--- Will mark test as failed but test will continue
---
--- @param ... any
M.log_error = function(...)
	tm:log("e", ...)
end

--- Print something to logs & TUI with 'warning' log level.
---
--- @param ... any
M.log_warning = function(...)
	tm:log("w", ...)
end

--- Print something to logs & TUI with 'info' log level.
---
--- @param ... any
M.log_info = function(...)
	tm:log("i", ...)
end

--- Print something to logs & TUI with 'debug' log level.
---
--- @param ... any
M.log_debug = function(...)
	tm:log("d", ...)
end

--- Print something to logs & TUI with 'trace' log level.
---
--- @param ... any
M.log_trace = function(...)
	tm:log("t", ...)
end

--- Put test to sleep for `ms` amount of milliseconds
---
--- @param ms number
M.sleep = function(ms)
	tm:sleep(ms)
end

return M
