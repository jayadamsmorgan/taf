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
---
--- @return string? error
M.session_end = function(session)
	return wd:session_end(session)
end

return M
