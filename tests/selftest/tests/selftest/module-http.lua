local taf = require("taf")

local check = require("test_checkup")
local util = require("util")

taf.test("Test module-http", { "module-http" }, function()
	local log_obj = util.load_log({ "test", "bootstrap", "-t", "module-http", "-e" })

	assert(log_obj.tags ~= nil)
	assert(#log_obj.tags == 1)
	assert(log_obj.tags[1] == "module-http")

	assert(log_obj.tests ~= nil)
	assert(#log_obj.tests == 4, "Expecteed 4 tests, got")

	local test = log_obj.tests[1]
	check.check_test(test, "Test HTTP POST request", "passed")
	util.test_tags(test, { "module-http" })
	util.error_if(#test.output ~= 1, test, "Outputs not match")
	check.check_output(test, test.output[1], '"form": {\n    "Hello from Lua!": ""\n  },', "INFO", true)

	test = log_obj.tests[2]
	check.check_test(test, "Test HTTP GET request", "passed")
	util.test_tags(test, { "module-http" })
	util.error_if(#test.output ~= 1, test, "Outputs not match")
	check.check_output(test, test.output[1], '"args": {},', "INFO", true)
	check.check_output(test, test.output[1], '"url": "https://httpbin.org/get"', "INFO", true)

	test = log_obj.tests[3]
	check.check_test(test, "Test HTTP DELETE request", "passed")
	util.test_tags(test, { "module-http" })
	util.error_if(#test.output ~= 1, test, "Outputs not match")
	check.check_output(test, test.output[1], '"args": {},', "INFO", true)

	test = log_obj.tests[4]
	check.check_test(test, "Test HTTP PUT request", "passed")
	util.test_tags(test, { "module-http" })
	util.error_if(#test.output ~= 1, test, "Outputs not match")
	check.check_output(test, test.output[1], '"args": {},', "INFO", true)
end)
