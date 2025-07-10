local taf = require("taf")

local check = require("test_checkup")
local util = require("util")

taf.test("Test module-webdriver", { "module-webdriver" }, function()
	local log_obj = util.load_log({ "test", "bootstrap", "-t", "module-webdriver", "-e" })

	assert(log_obj.tags ~= nil)
	assert(#log_obj.tags == 1)
	assert(log_obj.tags[1] == "module-webdriver")

	assert(log_obj.tests ~= nil)
	assert(#log_obj.tests == 1, "Expected 1 test, got " .. #log_obj.tests)

	local test = log_obj.tests[1]
	check.check_test(test, "Test minimal webdriver possibilities", "passed")
	util.test_tags(test, { "module-webdriver" })
	util.error_if(#test.output ~= 2, test, "Outputs not match")
	check.check_output(test, test.output[1], "https://github.com/", "INFO")
	check.check_output(
		test,
		test.output[2],
		"GitHub · Build and ship software on a single, collaborative platform · GitHub",
		"INFO"
	)
end)
