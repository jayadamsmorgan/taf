local taf = require("taf")
local proc = taf.proc

taf.test("Testing 'ls' command", function()
	local result, err = proc.run({ exe = "ls" })
	assert(not err, err or "unknown err")

	print("stdout: " .. result.stdout)

	print("Exited with code " .. result.exitcode)
end)
