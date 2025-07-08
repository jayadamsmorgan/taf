local M = {}

local taf = require("taf")
local util = require("util")

--- @alias status_t
--- | '"passed"'
--- | '"failed"'

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
		taf.log_error(expected_name .. " test is nil")
		return
	end

	if test.name == nil then
		taf.log_error(expected_name .. " test.name is nil")
	end
	if test.name ~= expected_name then
		taf.log_error(expected_name .. ("test.name is not expected: '%s'"):format(test.name))
	end

	if test.status == nil then
		taf.log_error(expected_name .. "test.status is nil")
	end
	if test.status ~= expected_status then
		taf.log_error(expected_name .. (" test.status is not expected: '%s'"):format(test.status))
	end
	if test.status == "failed" and test.failure_reasons == nil then
		taf.log_error(expected_name .. " test.status is 'failed' but test.failure_reasons is nil")
	end
	if test.status == "failed" and #test.failure_reasons == 0 then
		taf.log_error(expected_name .. " test.status is 'failed' but test.failure_reasons is empty")
	end

	if test.output == nil then
		taf.log_error(expected_name .. " test.output is nil")
	end

	if test.started == nil then
		taf.log_error(expected_name .. " test.started is nil")
	end
	if util.is_valid_datetime(test.started) == false then
		taf.log_error(expected_name .. (" test.started is not a vaild datetime: '%s'"):format(test.started))
	end
	if test.finished == nil then
		taf.log_error(expected_name .. " test.finished is nil")
	end
	if util.is_valid_datetime(test.finished) == false then
		taf.log_error(expected_name .. (" test.finished is not a vaild datetime: '%s'"):format(test.finished))
	end

	if test.tags == nil then
		taf.log_error(expected_name .. " test.tags is nil")
	end

	if test.teardown_output == nil then
		taf.log_error(expected_name .. "test.teardown_output is nil")
	end

	if test.teardown_errors == nil then
		taf.log_error(expected_name .. "test.teardown_errors is nil")
	end
end

--- @param output output_t?
--- @param expected_message string?
--- @param expected_level log_level_t?
---
--- @return string?, output_t?
M.check_output = function(output, expected_message, expected_level)
	if not output then
		return "output is nil", nil
	end

	if not output.date_time then
		return "output.date_time is nil", nil
	end

	if not output.file then
		return "output.file is nil", nil
	end

	if not output.level then
		return "output.level is nil", nil
	end
	if expected_level and expected_level ~= output.level then
		return ("output.level is not expected '%s'"):format(expected_level), nil
	end

	if output.msg == nil then
		return "output.msg is nil", nil
	end
	if expected_message and expected_message ~= output.msg then
		return ("output.msg is not expected '%s'"):format(expected_level), nil
	end

	return nil, output
end

return M
