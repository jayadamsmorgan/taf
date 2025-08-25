local M = {}

local taf = require("taf")
local util = require("util")

--- @alias status_t
--- | '"PASSED"'
--- | '"FAILED"'

--- @alias log_level_t
--- | '"CRITICAL"'
--- | '"ERROR"'
--- | '"WARNING"'
--- | '"INFO"'
--- | '"DEBUG"'
--- | '"TRACE"'

--- @class test_t
--- @field name string
--- @field started string
--- @field finished string
--- @field status status_t
--- @field tags [string]
--- @field output [output_t]
--- @field failure_reasons [output_t]? -- present only when status is "failed"
--- @field teardown_output [output_t]
--- @field teardown_errors [output_t]

--- @class output_t
--- @field file string
--- @field line integer
--- @field date_time string
--- @field level log_level_t
--- @field msg string

--- @param test test_t?
--- @param expected_name string
--- @param expected_status status_t
M.check_test = function(test, expected_name, expected_status)
	if test == nil then
		taf.log_error(("Test '%s': %s"):format(expected_name, "test is nil"))
		return
	end

	if test.name == nil then
		taf.log_error(("Test '%s': %s"):format(expected_name, "test.name is nil"))
		return
	end
	if test.name ~= expected_name then
		taf.log_error(
			("Test '%s': %s"):format(
				expected_name,
				("test.name is '%s', expected '%s"):format(test.name, expected_name)
			)
		)
		return
	end

	util.error_if(
		test.status ~= expected_status,
		test,
		("test.status is '%s', expected '%s'"):format(test.status, expected_status)
	)
	if test.status == "failed" and test.failure_reasons == nil then
		util.test_error(test, "test.status is 'failed' but test.failure_reasons is nil")
	end
	if test.status == "failed" and #test.failure_reasons == 0 then
		util.test_error(test, "test.status is 'failed' but test.failure_reasons is empty")
	end

	util.error_if(test.output == nil, test, "test.output is nil")
	util.error_if(
		test.started == nil or util.is_valid_datetime(test.started) == false,
		test,
		"test.started is nil or not valid"
	)
	util.error_if(
		test.finished == nil or util.is_valid_datetime(test.finished) == false,
		test,
		"test.finished is nil or not valid"
	)

	util.error_if(test.tags == nil, test, "test.tags is nil")
	util.error_if(test.teardown_output == nil, test, "test.teardown_output is nil")
	util.error_if(test.teardown_errors == nil, test, "test.teardown_errors is nil")
end

--- @param test test_t
--- @param output output_t?
--- @param expected_message string?
--- @param expected_level log_level_t?
--- @param contains boolean?
M.check_output = function(test, output, expected_message, expected_level, contains)
	contains = contains or false
	if not output then
		util.test_error(test, "output is nil")
		return
	end

	util.error_if(
		output.date_time == nil or util.is_valid_datetime(output.date_time) == false,
		test,
		"output.date_time is nil or not valid"
	)
	util.error_if(output.file == nil, test, "output.file is nil")
	util.error_if(#output.file == 0, test, "output.file is empty")
	util.error_if(output.line == nil, test, "output.line is nil")
	util.error_if(output.line == 0, test, "output.line is 0")
	util.error_if(output.level == nil, test, "output.level is nil")
	util.error_if(
		output.level ~= expected_level,
		test,
		("output.level is '%s', expected '%s'"):format(output.level, expected_level)
	)
	util.error_if(output.msg == nil, test, "output.msg is nil")
	if contains and expected_message then
		util.error_if(
			output.msg:find(expected_message, nil, true) == nil,
			test,
			("\noutput.msg is:\n'%s\ndoes not contain:\n'%s'"):format(output.msg, expected_message)
		)
	elseif expected_message then
		util.error_if(
			output.msg ~= expected_message,
			test,
			("output.msg is '%s', expected '%s'"):format(output.msg, expected_message)
		)
	end

	return nil, output
end

return M
