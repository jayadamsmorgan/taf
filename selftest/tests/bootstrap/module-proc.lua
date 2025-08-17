local taf = require("taf")
local proc = taf.proc

taf.test({
	name = "Test proc.run() existing binary",
	tags = { "module-proc" },
	body = function()
		local handle = proc.run({
			exe = "ls",
			args = { "-a" },
		})
		taf.log_info(handle.exitcode)
		taf.log_info(handle.stderr)
		taf.log_info(handle.stdout)
	end,
})

taf.test({
	name = "Test proc.run() non-existent binary",
	tags = { "module-proc" },
	body = function()
		-- Should throw
		proc.run({
			exe = "some_binary",
			args = {
				"some",
				"arguments",
			},
		})
	end,
})

taf.test({
	name = "Test proc.run() with timeout not triggered",
	tags = { "module-proc" },
	body = function()
		-- Timeout is 1000ms and we are sleeping for 500ms
		local handle = proc.run({
			exe = "sleep",
			args = { "0.5" },
		}, 1000)
		taf.log_info(handle.exitcode)
		taf.log_info(handle.stderr)
		taf.log_info(handle.stdout)
	end,
})

taf.test({
	name = "Test proc.run() with timeout triggered",
	tags = { "module-proc" },
	body = function()
		-- Should throw (timeout is 250ms and we are sleeping for 500ms)
		proc.run({
			exe = "sleep",
			args = { "0.5" },
		}, 250)
	end,
})

taf.test({
	name = "Test proc:wait()",
	tags = { "module-proc" },
	body = function()
		local handle = proc.spawn({
			exe = "sleep",
			args = { "0.5" },
		})
		taf.defer(function()
			handle:kill()
		end)
		local status = handle:wait()
		taf.log_info(status)
		taf.sleep(1000)
		status = handle:wait()
		taf.log_info(status)
	end,
})

taf.test({
	name = "Test proc.spawn() with non-existent binary",
	tags = { "module-proc" },
	body = function()
		-- Should throw
		proc.spawn({
			exe = "some_binary",
			args = { "some", "arguments" },
		})
	end,
})

taf.test({
	name = "Test proc:read() stdio",
	tags = { "module-proc" },
	body = function()
		local handle = proc.spawn({
			exe = "echo",
			args = { "hello" },
		})
		taf.defer(function()
			handle:kill()
		end)
		taf.sleep(100) -- just to make sure
		local result = handle:read()
		taf.log_info(result)
	end,
})

taf.test({
	name = "Test proc:read() stderr",
	tags = { "module-proc" },
	body = function()
		local handle = proc.spawn({
			exe = "sleep",
			args = { "some_argument" },
		})
		taf.defer(function()
			handle:kill()
		end)
		taf.sleep(100) -- just to make sure
		local result = handle:read("stderr")
		taf.log_info(result)
	end,
})
