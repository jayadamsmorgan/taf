local wd = require("taf-webdriver")

local M = {}

--- @alias driver
--- | '"chromedriver"'
--- | '"geckodriver"'
--- | '"safaridriver"'
--- | '"msedgedriver"'
--- | '"iedriver"'
--- | '"operadriver"'
--- | '"webkitdriver"'
--- | '"wpedriver"'

--- @alias api_method
--- | '"put"'
--- | '"post"'
--- | '"delete"'
--- | '"get"'

--- @alias session userdata

--- @param driver_port integer
--- @param backend driver?
--- @param extraflags string?
---
--- @return session
M.session_start = function(driver_port, backend, extraflags)
	backend = backend or "chromedriver"
	return wd:session_start(driver_port, backend, extraflags)
end

--- @param session session
--- @param url string
---
--- @return string? error
M.open_url = function(session, url)
	local payload = {
		url = url,
	}
	local payload, err = M.session_cmd(session, "put", "url", payload)
	if err then
		return err
	end

	return err
end

--- @param session session
--- @param method api_method
--- @param endpoint string
--- @param payload table
---
--- @return table? response, string? error
M.session_cmd = function(session, method, endpoint, payload)
	wd:session_cmd(session, method, endpoint, payload)
end

--- @param session session
---
--- @return string? error
M.session_end = function(session)
	return wd:session_end(session)
end

return M
