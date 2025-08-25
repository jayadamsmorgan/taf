local taf = require("taf")

local check = require("test_checkup")
local util = require("util")

taf.test({
	name = "Test module-json",
	tags = { "module-json" },
	body = function()
		local log_obj = util.load_log({ "test", "bootstrap", "-t", "module-json" })

		assert(log_obj.tags ~= nil)
		assert(#log_obj.tags == 1)
		assert(log_obj.tags[1] == "module-json")

		assert(log_obj.tests ~= nil)
		assert(#log_obj.tests == 3, "Expected 3 tests, got " .. #log_obj.tests)

		local test = log_obj.tests[1]
		check.check_test(test, "Test JSON serialization", "PASSED")
		util.test_tags(test, { "module-json" })
		util.error_if(#test.output ~= 1, test, "Outputs not match")
		check.check_output(test, test.output[1], '"another_object":{"string":"test_string"}', "INFO", true)
		check.check_output(test, test.output[1], '"int_array":[1,2,3,4]', "INFO", true)
		check.check_output(test, test.output[1], '"integer":2', "INFO", true)
		check.check_output(test, test.output[1], '"double":3.1400000000000001', "INFO", true)
		check.check_output(test, test.output[1], '"map":{', "INFO", true)
		check.check_output(test, test.output[1], '"str_array":["1","2","3","4"]', "INFO", true)

		test = log_obj.tests[2]
		check.check_test(test, "Test JSON deserialization with good string", "PASSED")
		util.test_tags(test, { "module-json" })
		util.error_if(#test.output ~= 5, test, "Outputs not match")
		check.check_output(test, test.output[1], "2.0", "INFO")
		check.check_output(test, test.output[2], "3.14", "INFO")
		check.check_output(test, test.output[3], "test_string", "INFO")
		check.check_output(test, test.output[4], "10.0", "INFO")
		check.check_output(test, test.output[5], "1234", "INFO")

		test = log_obj.tests[3]
		check.check_test(test, "Test JSON deserialization with bad string", "FAILED")
		util.test_tags(test, { "module-json" })
		util.error_if(#test.output ~= 0, test, "Outputs not match")
		check.check_output(test, test.failure_reasons[1], "stack traceback:", "CRITICAL", true)
	end,
})
