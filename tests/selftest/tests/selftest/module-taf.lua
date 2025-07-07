local taf = require("taf")
local json = taf.json
local proc = taf.proc

taf.test("Test raw log file is present", { "module-taf" }, function()
	local proc_handle = proc.spawn({
		exe = "taf",
		args = {
			"test",
			"bootstrap",
			"-t",
			"module-taf",
			"-e",
		},
	})
	taf.defer(function()
		proc_handle:kill()
	end)
	while proc_handle:wait() == nil do
		proc_handle:read() -- flush stdout to not hang on large buffers
	end

	local log_file = io.open("logs/bootstrap/test_run_latest_raw.json", "r")

	assert(log_file)

	taf.defer(function()
		log_file:close()
	end)

	local str = log_file:read("a")
	assert(str)
	assert(#str ~= 0)

	local log_obj = json.deserialize(str)

	assert(log_obj.taf_version ~= nil)
	assert(log_obj.taf_version ~= "")
	taf.log_info("TAF version:", log_obj.taf_version)

	assert(log_obj.os ~= nil)
	assert(log_obj.os ~= "")
	taf.log_info("OS:", log_obj.os)

	assert(log_obj.os_version ~= nil)
	assert(log_obj.os_version ~= "")
	taf.log_info("OS version:", log_obj.os_version)

	assert(log_obj.started ~= nil)
	assert(log_obj.started ~= "")
	taf.log_info("Test run started:", log_obj.started)

	assert(log_obj.finished ~= nil)
	assert(log_obj.finished ~= "")
	taf.log_info("Test run finished:", log_obj.finished)

	assert(log_obj.tags ~= nil)
	assert(#log_obj.tags == 1)
	assert(log_obj.tags[1] == "module-taf")
	taf.log_info("Test run tag is '" .. log_obj.tags[1] .. "'")

	assert(log_obj.tests ~= nil)
	assert(#log_obj.tests == 17)
end)
