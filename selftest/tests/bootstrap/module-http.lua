local taf = require("taf")
local http = taf.http

taf.test({
	name = "Test HTTP POST request",
	tags = { "module-http" },
	body = function()
		local handle = http.new()
		taf.defer(function()
			handle:cleanup()
		end)

		local src = "Hello from Lua!"
		local pos = 1

		local buffer = ""

		handle
			:setopt(http.OPT_URL, "https://httpbin.org/post")
			:setopt(http.OPT_POST, 1)
			:setopt(http.OPT_INFILESIZE, #src)
			:setopt(http.OPT_READFUNCTION, function(max)
				local chunk = src:sub(pos, pos + max - 1)
				pos = pos + #chunk
				return chunk
			end)
			:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
				buffer = buffer .. chunk
				return n
			end)
			:perform()

		taf.log_info(buffer)
	end,
})

taf.test({
	name = "Test HTTP GET request",
	tags = { "module-http" },
	body = function()
		local handle = http.new()
		taf.defer(function()
			handle:cleanup()
		end)

		local buffer = ""

		handle
			:setopt(http.OPT_URL, "https://httpbin.org/get")
			:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
				buffer = buffer .. chunk
				return n
			end)
			:perform()

		taf.log_info(buffer)
	end,
})

taf.test({
	name = "Test HTTP DELETE request",
	tags = { "module-http" },
	body = function()
		local handle = http.new()
		taf.defer(function()
			handle:cleanup()
		end)

		local result = ""
		handle
			:setopt(http.OPT_URL, "https://httpbin.org/delete")
			:setopt(http.OPT_CUSTOMREQUEST, "DELETE")
			:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
				result = result .. chunk
				return n
			end)
			:perform()

		taf.log_info(result)
	end,
})

taf.test({
	name = "Test HTTP PUT request",
	tags = { "module-http" },
	body = function()
		local handle = http.new()
		taf.defer(function()
			handle:cleanup()
		end)

		local result = ""
		handle
			:setopt(http.OPT_URL, "https://httpbin.org/put")
			:setopt(http.OPT_CUSTOMREQUEST, "PUT")
			:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
				result = result .. chunk
				return n
			end)
			:perform()

		taf.log_info(result)
	end,
})
