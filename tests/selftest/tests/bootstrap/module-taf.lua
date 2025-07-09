local taf = require("taf")

taf.test("Test logging with 'trace' log level", { "module-taf", "logging" }, function()
	local str = "Testing logging with TRACE log level."
	taf.log("TRACE", str)
	taf.log("trace", str)
	taf.log("T", str)
	taf.log("t", str)
	taf.log_trace(str)
end)

taf.test("Test logging with 'debug' log level", { "module-taf", "logging" }, function()
	local str = "Testing logging with DEBUG log level."
	taf.log("DEBUG", str)
	taf.log("debug", str)
	taf.log("D", str)
	taf.log("d", str)
	taf.log_debug(str)
end)

taf.test("Test logging with 'info' log level", { "module-taf", "logging" }, function()
	local str = "Testing logging with INFO log level."
	taf.log("INFO", str)
	taf.log("info", str)
	taf.log("I", str)
	taf.log("i", str)
	taf.log_info(str)
end)

taf.test("Test logging with 'warning' log level", { "module-taf", "logging" }, function()
	local str = "Testing logging with WARNING log level."
	taf.log("WARNING", str)
	taf.log("warning", str)
	taf.log("W", str)
	taf.log("w", str)
	taf.log_warning(str)
end)

taf.test("Test logging with 'error' log level", { "module-taf", "logging" }, function()
	-- Test should fail but execute everything
	local str = "Testing logging with ERROR log level."
	taf.log("ERROR", str)
	taf.log("error", str)
	taf.log("E", str)
	taf.log("e", str)
	taf.log_error(str)
end)

-- Tests for critical log level are separate because this level stops test execution
taf.test("Test logging with 'critical' log level ('CRITICAL')", { "module-taf", "logging" }, function()
	local str = "Testing logging with CRITICAL log level."
	taf.log("CRITICAL", str)
end)

taf.test("Test logging with 'critical' log level ('critical')", { "module-taf", "logging" }, function()
	local str = "Testing logging with CRITICAL log level."
	taf.log("critical", str)
end)

taf.test("Test logging with 'critical' log level ('C')", { "module-taf", "logging" }, function()
	local str = "Testing logging with CRITICAL log level."
	taf.log("C", str)
end)

taf.test("Test logging with 'critical' log level ('c')", { "module-taf", "logging" }, function()
	local str = "Testing logging with CRITICAL log level."
	taf.log("c", str)
end)

taf.test("Test logging with 'critical' log level ('taf.log_critical')", { "module-taf", "logging" }, function()
	local str = "Testing logging with CRITICAL log level."
	taf.log_critical(str)
end)

taf.test("Test logging with incorrect log level", { "module-taf", "logging" }, function()
	-- This test should throw on the next line:
	---@diagnostic disable-next-line: param-type-mismatch
	taf.log("incorrect", "Testing logging with incorrect log level.")
end)

taf.test("Test logging with multiple arguments", { "module-taf", "logging" }, function()
	taf.log_info("test1", "test2", "test3", "test4 test5")
end)

taf.test("Test Lua's 'print' is forwarded to logging with INFO log level", { "module-taf", "logging" }, function()
	print("Testing logging")
	taf.print("Testing logging")
end)

taf.test("Test taf.sleep", { "module-taf" }, function()
	taf.sleep(10)
end)

taf.test("Test taf.get_current_target", { "module-taf" }, function()
	local target = taf.get_current_target()
	taf.log("INFO", target)
	assert(target == "bootstrap")
end)

taf.test("Test taf.defer", { "module-taf" }, function()
	-- Simple
	taf.defer(function()
		taf.log_info("defer 1")
	end)

	-- Status argument
	taf.defer(function(status)
		taf.log_info("defer 2")
		assert(status == "passed")
	end)

	-- Failing inside of defer
	taf.defer(function()
		taf.log_info("defer 3")
		assert(nil) -- Should fail here
	end)

	-- One more to be sure :)
	taf.defer(function()
		taf.log_info("defer 4")
	end)

	-- The alternative syntax
	taf.defer(taf.log_info, "defer 5")
end)

taf.test("Test taf.millis", { "module-taf" }, function()
	taf.log_info(taf.millis())
	taf.sleep(10)
	taf.log_info(taf.millis())
end)
