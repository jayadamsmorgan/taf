local tm = require("taf-main")

local M = {}

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
--- | '"WARNING"'
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

-- Expose submodules of 'taf'
M.serial = require("taf.serial")
M.webdriver = require("taf.webdriver")
M.proc = require("taf.proc")
M.json = require("taf.json")
M.http = require("taf.http")
M.hooks = require("taf.hooks")

--- Get amount of milliseconds since test started
---
--- @return number ms
M.millis = function()
	return tm:millis()
end

--- @class taf_test_opts
--- @field name string name of the test
--- @field description string? description of the test, optional
--- @field tags [string]? array of test tags, optional
--- @field body fun() body of the test

--- Register new test
---
--- @param opts taf_test_opts
M.test = function(opts)
	tm.test(opts)
end

--- @param defer_func function function executed on defer
--- @param ... any arguments to pass to the function
M.defer = function(defer_func, ...)
	tm:defer(defer_func, ...)
end

--- Get currently active tags for the test run
---
--- @return [string] tags
M.get_active_tags = function()
	return tm:get_active_tags()
end

--- Get currently active tags for the current test only
---
--- @return [string] tags
M.get_active_test_tags = function()
	return tm:get_active_test_tags()
end

--- Get current target name if project is multitarget. Returns an empty string otherwise.
---
--- @return string target
M.get_current_target = function()
	return tm:get_current_target()
end

--- @class taf_var_reg_t
--- @field values [string]?
--- @field default string?

--- Register variables
---
--- @param vars table<string, taf_var_reg_t|string>
M.register_vars = function(vars)
	tm.register_vars(vars)
end

--- @class taf_var_t
--- @field name string
--- @field value string

--- Returns all registered variables
---
--- @return [taf_var_t]
M.get_vars = function()
	return tm:get_vars()
end

--- Returns variable with the specified name
---
--- @param var_name string
---
--- @return taf_var_t
M.get_var = function(var_name)
	return tm:get_var(var_name)
end

--- @class taf_secret_t
--- @field name string
--- @field value string

--- Register secrets
---
--- @param secrets [string]
M.register_secrets = function(secrets)
	tm.register_secrets(secrets)
end

--- Returns secret variable with the specified name
---
--- @param secret_name string
---
--- @return taf_secret_t
M.get_secret = function(secret_name)
	return tm:get_secret(secret_name)
end

--- Print something to logs & TUI. Same as default Lua `print()`. Both will use 'info' log level
---
--- @param ... any
M.print = function(...)
	tm:print(...)
end

--- Print something to logs & TUI with specified log level.
--- If `log_level` is "critical" - fails test immedeately.
--- If `log_level` is "error" - fails test but continues to execute.
--- Other `log_level` will just log.
---
--- @param log_level log_level
--- @param ... any
M.log = function(log_level, ...)
	tm:log(log_level, ...)
end

--- Print something to logs & TUI with 'critical' log level.
--- Will fail test immedeately.
---
--- @param ... any
M.log_critical = function(...)
	tm:log("c", ...)
end

--- Print something to logs & TUI with 'error' log level.
--- Will mark test as failed but test will continue.
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

--- Put test to sleep for `ms` amount of milliseconds.
---
--- @param ms number
M.sleep = function(ms)
	tm:sleep(ms)
end

--- Context types for hooks:

--- @class context_t
--- @field test_run test_run_context_t
--- @field test test_context_t
--- @field log_dir string

--- @class test_run_context_t
--- @field project_name string
--- @field taf_version string
--- @field started string
--- @field finished string?
--- @field os string
--- @field os_version string
--- @field target string?
--- @field tags [string]

--- @class test_output_t
--- @field file string
--- @field line integer
--- @field date_time string
--- @field level "CRITICAL"|"ERROR"|"WARNING"|"INFO"|"DEBUG"|"TRACE"
--- @field msg string

--- @class test_context_t
--- @field test_file string
--- @field name string
--- @field started string
--- @field finished string?
--- @field status "passed"|"failed"|?
--- @field tags [string]
--- @field output [test_output_t]
--- @field failure_reasons [test_output_t]
--- @field teardown_output [test_output_t]
--- @field teardown_errors [test_output_t]

return M
