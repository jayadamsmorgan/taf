local taf = require("taf")
local json = taf.json
local proc = taf.proc

local check = require("test_checkup")
local util = require("util")

taf.test("Test module-taf", { "module-taf" }, function()
	local proc_handle = proc.spawn({
		exe = "taf",
		args = {
			"test",
			"bootstrap",
			"-t",
			"module-taf,common",
			"-e",
		},
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
	taf.log_info("TAF version:", log_obj.taf_version)

	assert(log_obj.os ~= nil)
	assert(log_obj.os == "macos" or log_obj.os == "linux")

	assert(log_obj.os_version ~= nil)
	assert(log_obj.os_version ~= "")
	taf.log_info("OS version:", log_obj.os_version)

	assert(log_obj.started ~= nil)
	assert(util.is_valid_datetime(log_obj.started))

	assert(log_obj.finished ~= nil)
	assert(util.is_valid_datetime(log_obj.finished))

	assert(log_obj.tags ~= nil)
	assert(#log_obj.tags == 2)
	assert(log_obj.tags[1] == "module-taf")
	assert(log_obj.tags[2] == "common")

	assert(log_obj.tests ~= nil)
	assert(#log_obj.tests == 18)

	local test = log_obj.tests[1]
	check.check_test(log_obj.tests[1], "Test common TAF test", "passed")
	if #test.tags ~= 1 and test.tags[1] ~= "common" then
		taf.log_error(test.name .. " Tags not match")
	end
	if #test.output ~= 1 then
		taf.log_error(test.name .. "Outputs not match")
	else
		check.check_output(test.output[1], "common", "INFO")
	end

	test = log_obj.tests[2]
	check.check_test(test, "Test logging with 'trace' log level", "passed")
	if #test.tags ~= 2 and test.tags[1] ~= "module-taf" and test.tags[2] ~= "logging" then
		taf.log_error(test.name .. " Tags not match")
	end
	if #test.output ~= 5 then
		taf.log_error(test.name .. " Outputs not match")
	end
	for i = 1, 5 do
		check.check_output(test.output[i], "Testing logging with TRACE log level.", "TRACE")
	end

	test = log_obj.tests[3]
end)
