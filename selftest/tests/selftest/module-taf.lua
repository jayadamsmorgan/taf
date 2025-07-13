local taf = require("taf")

local check = require("test_checkup")
local util = require("util")

taf.test("Test module-taf (common)", { "module-taf", "common" }, function()
	local log_obj = util.load_log({ "test", "bootstrap", "-t", "common", "-e" })

	assert(log_obj.tags ~= nil)
	assert(#log_obj.tags == 1)
	assert(log_obj.tags[1] == "common")

	assert(log_obj.tests ~= nil)
	assert(#log_obj.tests == 1, "Expected 1 test, got " .. #log_obj.tests)

	local test = log_obj.tests[1]

	check.check_test(test, "Test common TAF test", "passed")
	util.test_tags(test, { "common" })
	util.error_if(#test.output ~= 1, test, "Outputs not match")
	if #test.output == 1 then
		check.check_output(test, test.output[1], "common", "INFO")
	end
end)

taf.test("Test module-taf (logging)", { "module-taf", "logging" }, function()
	local log_obj = util.load_log({ "test", "bootstrap", "-t", "logging", "-e" })

	assert(log_obj.tags ~= nil)
	assert(#log_obj.tags == 1)
	assert(log_obj.tags[1] == "logging")

	assert(log_obj.tests ~= nil)
	assert(#log_obj.tests == 13, "Expected 13 tests, got " .. #log_obj.tests)

	--- @param log_test test_t
	--- @param log_level log_level_t
	--- @param amount integer
	--- @param status status_t
	--- @param additional string?
	local function test_logging_tests(log_test, log_level, amount, status, additional)
		local add = additional or ""
		check.check_test(log_test, ("Test logging with '%s' log level%s"):format(log_level:lower(), add), status)
		util.test_tags(log_test, { "module-taf", "logging" })
		util.error_if(#log_test.output ~= amount, log_test, "Outputs not match")
		local str_to_find = ("Testing logging with %s log level."):format(log_level)
		if status == "failed" then
			util.error_if(#log_test.failure_reasons ~= amount, log_test, "Outputs not match")
		end
		for i = 1, amount do
			check.check_output(log_test, log_test.output[i], str_to_find, log_level)
			if status == "failed" then
				local contains = log_level == "CRITICAL"
				-- When log level is CRITICAL failure reason will have a traceback as a message,
				-- hence we are trying to find message inside traceback with `contains`
				check.check_output(log_test, log_test.failure_reasons[i], str_to_find, log_level, contains)
				if contains then
					-- Make sure traceback is there also
					-- We just shouldn't log anything with this word in bootstrap
					util.error_if(
						log_test.failure_reasons[i].msg:find("stack traceback:") == nil,
						log_test,
						"Unable to find traceback"
					)
				end
			end
		end
	end

	test_logging_tests(log_obj.tests[1], "TRACE", 5, "passed")
	test_logging_tests(log_obj.tests[2], "DEBUG", 5, "passed")
	test_logging_tests(log_obj.tests[3], "INFO", 5, "passed")
	test_logging_tests(log_obj.tests[4], "WARNING", 5, "passed")
	test_logging_tests(log_obj.tests[5], "ERROR", 5, "failed")
	test_logging_tests(log_obj.tests[6], "CRITICAL", 1, "failed", " ('CRITICAL')")
	test_logging_tests(log_obj.tests[7], "CRITICAL", 1, "failed", " ('critical')")
	test_logging_tests(log_obj.tests[8], "CRITICAL", 1, "failed", " ('C')")
	test_logging_tests(log_obj.tests[9], "CRITICAL", 1, "failed", " ('c')")
	test_logging_tests(log_obj.tests[10], "CRITICAL", 1, "failed", " ('taf.log_critical')")

	local test = log_obj.tests[11]
	check.check_test(test, "Test logging with incorrect log level", "failed")
	util.test_tags(test, { "module-taf", "logging" })
	util.error_if(#test.output ~= 0, test, "Outputs not match")
	util.error_if(#test.failure_reasons ~= 1, test, "Outputs not match")
	check.check_output(test, test.failure_reasons[1], "Unknown log level 'incorrect'", "CRITICAL", true)
	util.error_if(test.failure_reasons[1].msg:find("stack traceback:") == nil, test, "Unable to find traceback")

	test = log_obj.tests[12]
	check.check_test(test, "Test logging with multiple arguments", "passed")
	util.test_tags(test, { "module-taf", "logging" })
	util.error_if(#test.output ~= 1, test, "Outputs not match")
	check.check_output(test, test.output[1], "test1\ttest2\ttest3\ttest4 test5", "INFO")

	test = log_obj.tests[13]
	check.check_test(test, "Test Lua's 'print' is forwarded to logging with INFO log level", "passed")
	util.test_tags(test, { "module-taf", "logging" })
	util.error_if(#test.output ~= 2, test, "Outputs not match")
	for i = 1, 2 do
		check.check_output(test, test.output[i], "Testing logging", "INFO")
	end
end)

taf.test("Test module-taf (utils)", { "module-taf", "utils" }, function()
	local log_obj = util.load_log({ "test", "bootstrap", "-t", "utils", "-e" })

	assert(log_obj.tags ~= nil)
	assert(#log_obj.tags == 1)
	assert(log_obj.tags[1] == "utils")

	assert(log_obj.tests ~= nil)
	assert(#log_obj.tests == 4, "Expected 4 tests, got " .. #log_obj.tests)

	local test = log_obj.tests[1]
	check.check_test(test, "Test taf.sleep", "passed")
	util.test_tags(test, { "module-taf", "utils" })
	util.error_if(test.finished == test.started, test, "No sleep")

	test = log_obj.tests[2]
	check.check_test(test, "Test taf.get_current_target", "passed")
	util.test_tags(test, { "module-taf", "utils" })
	util.error_if(#test.output ~= 1, test, "Outputs not match")
	check.check_output(test, test.output[1], "bootstrap", "INFO")

	test = log_obj.tests[3]
	check.check_test(test, "Test taf.defer", "passed")
	util.test_tags(test, { "module-taf", "utils" })
	util.error_if(#test.output ~= 0, test, "Outputs not match")
	util.error_if(#test.teardown_output ~= 5, test, "Outputs not match")
	local j = 1
	for i = 5, 1, -1 do
		-- We check from last to first, so `j` should go up
		check.check_output(test, test.teardown_output[i], "defer " .. j, "INFO")
		j = j + 1
	end
	util.error_if(#test.teardown_errors ~= 1, test, "Outputs not match")
	check.check_output(test, test.teardown_errors[1], "assertion failed", "CRITICAL", true)

	test = log_obj.tests[4]
	check.check_test(test, "Test taf.millis", "passed")
	util.test_tags(test, { "module-taf", "utils" })
	util.error_if(#test.output ~= 2, test, "Outputs not match")
	check.check_output(test, test.output[1], nil, "INFO")
	check.check_output(test, test.output[2], nil, "INFO")
	local start_ms = tonumber(test.output[1].msg)
	util.error_if(start_ms == nil, test, "start_ms is nil")
	local end_ms = tonumber(test.output[2].msg)
	util.error_if(end_ms == nil, test, "end_ms is nil")
	if start_ms and end_ms then
		local time = end_ms - start_ms
		util.error_if(time < 10 or time > 50, test, "Incorrect millis " .. time)
	end
end)
