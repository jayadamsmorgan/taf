local M = {}

local taf = require("taf")
local proc = taf.proc
local json = taf.json

--- @param s string
--- @return boolean
M.is_valid_datetime = function(s)
	local MM, DD, YY, hh, mm, ss = s:match("^(%d%d)%.(%d%d)%.(%d%d)%-(%d%d):(%d%d):(%d%d)$")
	if not MM then
		return false
	end

	MM, DD, YY, hh, mm, ss = tonumber(MM), tonumber(DD), tonumber(YY), tonumber(hh), tonumber(mm), tonumber(ss)

	if MM < 1 or MM > 12 then
		return false
	end
	if hh > 23 or mm > 59 or ss > 59 then
		return false
	end

	local dim = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	if YY % 4 == 0 then
		dim[2] = 29
	end

	if DD < 1 or DD > dim[MM] then
		return false
	end

	return true
end

--- @param test test_t
--- @param message string
M.test_error = function(test, message)
	taf.log_error(("Test '%s': %s"):format(test.name, message))
end

--- @param condition boolean
--- @param test test_t
--- @param message string
M.error_if = function(condition, test, message)
	if condition == true then
		M.test_error(test, message)
	end
end

--- @param tag_test test_t
--- @param tag_array [string]
M.test_tags = function(tag_test, tag_array)
	if tag_test.tags == nil then
		return
	end
	M.error_if(#tag_test.tags ~= #tag_array, tag_test, "Tags not match")
	for i = 1, #tag_array do
		M.error_if(
			tag_test.tags[i] ~= tag_array[i],
			tag_test,
			("Tags not match: expected '%s' but got '%s'"):format(tag_array[i], tag_test.tags[i])
		)
	end
end

--- @param args [string]
--- @return table
M.load_log = function(args)
	local proc_handle = proc.spawn({
		exe = "taf",
		args = args,
	})
	while proc_handle:wait() == nil do
		proc_handle:read() -- flush stdout to not hang on large buffers
	end
	proc_handle:kill()

	local log_file = io.open("logs/bootstrap/test_run_latest_raw.json", "r")

	assert(log_file)

	local str = log_file:read("a")
	log_file:close()
	assert(str)
	assert(#str ~= 0)

	local log_obj = json.deserialize(str)

	assert(log_obj.taf_version ~= nil)
	assert(log_obj.taf_version ~= "")

	assert(log_obj.os ~= nil)
	assert(log_obj.os == "macos" or log_obj.os == "linux")

	assert(log_obj.os_version ~= nil)
	assert(log_obj.os_version ~= "")

	assert(log_obj.started ~= nil)
	assert(M.is_valid_datetime(log_obj.started))

	assert(log_obj.finished ~= nil)
	assert(M.is_valid_datetime(log_obj.finished))

	return log_obj
end

return M
