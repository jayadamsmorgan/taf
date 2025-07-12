local taf = require("taf")

local check = require("test_checkup")
local util = require("util")

taf.test("Test module-proc", { "module-proc" }, function()
	local log_obj = util.load_log({ "test", "bootstrap", "-t", "module-proc", "-e" })

	assert(log_obj.tags ~= nil)
	assert(#log_obj.tags == 1)
	assert(log_obj.tags[1] == "module-proc")

	assert(log_obj.tests ~= nil)
	assert(#log_obj.tests == 8)

	local test = log_obj.tests[1]
	check.check_test(test, "Test proc.run() existing binary", "passed")
	util.test_tags(test, { "module-proc" })
	util.error_if(#test.output ~= 3, test, "Outputs not match")
	check.check_output(test, test.output[1], "0", "INFO")
	check.check_output(test, test.output[2], "", "INFO")
	check.check_output(test, test.output[3], ".taf.json", "INFO", true)

	test = log_obj.tests[2]
	check.check_test(test, "Test proc.run() non-existent binary", "failed")
	util.test_tags(test, { "module-proc" })
	util.error_if(#test.output ~= 0, test, "Outputs not match")
	util.error_if(#test.failure_reasons ~= 1, test, "Outputs not match")
	check.check_output(test, test.failure_reasons[1], "No such file or directory\nstack traceback:", "CRITICAL", true)

	test = log_obj.tests[3]
	check.check_test(test, "Test proc.run() with timeout not triggered", "passed")
	util.test_tags(test, { "module-proc" })
	util.error_if(#test.output ~= 3, test, "Outputs not match")
	check.check_output(test, test.output[1], "0", "INFO")
	check.check_output(test, test.output[2], "", "INFO")
	check.check_output(test, test.output[3], "", "INFO")

	test = log_obj.tests[4]
	check.check_test(test, "Test proc.run() with timeout triggered", "failed")
	util.test_tags(test, { "module-proc" })
	util.error_if(#test.failure_reasons ~= 1, test, "Outputs not match")
	check.check_output(test, test.failure_reasons[1], "timeout\nstack traceback:", "CRITICAL", true)

	test = log_obj.tests[5]
	check.check_test(test, "Test proc:wait()", "passed")
	util.test_tags(test, { "module-proc" })
	util.error_if(#test.output ~= 2, test, "Outputs not match")
	check.check_output(test, test.output[1], "nil", "INFO")
	check.check_output(test, test.output[2], "0", "INFO")

	test = log_obj.tests[6]
	check.check_test(test, "Test proc.spawn() with non-existent binary", "failed")
	util.test_tags(test, { "module-proc" })
	util.error_if(#test.failure_reasons ~= 1, test, "Outputs not match")
	check.check_output(test, test.failure_reasons[1], "No such file or directory\nstack traceback:", "CRITICAL", true)

	test = log_obj.tests[7]
	check.check_test(test, "Test proc:read() stdio", "passed")
	util.test_tags(test, { "module-proc" })
	util.error_if(#test.output ~= 1, test, "Output not match")
	check.check_output(test, test.output[1], "hello\n", "INFO")

	test = log_obj.tests[8]
	check.check_test(test, "Test proc:read() stderr", "passed")
	util.test_tags(test, { "module-proc" })
	util.error_if(#test.output ~= 1, test, "Outputs not match")
	check.check_output(test, test.output[1], "sleep: invalid time interval", "INFO", true)
end)
