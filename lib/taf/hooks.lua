local th = require("taf-hooks")

local M = {}

--- @alias hooks_fn fun(context: context_t)

--- Register 'test_run_started' hook
--- @param fn hooks_fn
M.test_run_started = function(fn)
	th:register_test_run_started(fn)
end

--- Register 'test_started' hook
--- @param fn hooks_fn
M.test_started = function(fn)
	th:register_test_started(fn)
end

--- Register 'test_started' hook
--- @param fn hooks_fn
M.test_finished = function(fn)
	th:register_test_finished(fn)
end

--- Register 'test_run_finished' hook
--- @param fn hooks_fn
M.test_run_finished = function(fn)
	th:register_test_run_finished(fn)
end

return M
