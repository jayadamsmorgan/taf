local proc = require("taf-proc")
local tm = require("taf-main")

local M = {}

M.low = proc

--- @class run_opts
--- @field exe string the path to executable or its name if it's on PATH
--- @field args [string]? optional array of arguments to the executable

--- @alias proc_output_stream
--- | '"stdout"'
--- | '"stderr"'

--- @alias proc_read_func fun(self:proc_handle, want: integer?, stream:proc_output_stream?):string
--- @alias proc_write_func fun(self:proc_handle, buf:string):integer
--- @alias proc_wait_func fun(self:proc_handle):integer?
--- @alias proc_kill_func fun(self:proc_handle)

--- @class proc_handle
--- @field read proc_read_func read stdout/stderr from the spawned process. must not be called after `kill` (`want`: amount of bytes expected (4096 default), `stream`: which stream to read (stdout default))
--- @field write proc_write_func write buffer to stdin to the spawned process if it is still alive (`buf`: buffer to write, returns amount of bytes written)
--- @field wait proc_wait_func nonblocking function to check on current status of process (nil if still running)
--- @field kill proc_kill_func send SIGINT signal to process if it's still running

--- @param opts run_opts
---
--- @return proc_handle
M.spawn = function(opts)
	local argv = opts.args or {}
	table.insert(argv, 1, opts.exe)
	return proc:spawn(argv)
end

--- @class run_result
--- @field stdout string
--- @field stderr string
--- @field exitcode integer

--- @param opts run_opts
--- @param timeout integer? timeout in milliseconds. keep nil for indefinite waiting
--- @param sleepinterval integer? interval in milliseconds between checking for timeout. Default: 20
---
--- @return run_result result, string? err
M.run = function(opts, timeout, sleepinterval)
	local handle = M.spawn(opts)
	local status = nil
	if timeout then
		local start = tm:millis()
		local deadline = start + timeout
		local interval = sleepinterval or 20
		while status == nil do
			if tm:millis() >= deadline then
				return {
					stdout = "",
					stderr = "",
					exitcode = -1,
				}, "timeout"
			end
			tm:sleep(interval)
			status = handle:wait()
		end
	else
		while status == nil do
			status = handle:wait()
		end
	end
	local stdout = handle:read()
	local stderr = handle:read(nil, "stderr")
	handle:kill()
	return {
		stdout = stdout,
		stderr = stderr,
		exitcode = status,
	}, nil
end

return M
