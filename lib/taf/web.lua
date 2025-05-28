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

--- Start webdriver session
---
--- @param driver_port integer port to open webdriver with
--- @param backend driver?
--- @param extraflags [string]?
---
--- @return session
M.session_start = function(driver_port, backend, extraflags)
	backend = backend or "chromedriver"
	extraflags = extraflags or {}
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
	local _, err = M.session_cmd(session, "post", "url", payload)
	if err then
		return err
	end

	return err
end

--- Go back in history.
M.go_back = function(session)
	local _, err = M.session_cmd(session, "post", "back", {})
	return err
end

--- Go forward in history.
M.go_forward = function(session)
	local _, err = M.session_cmd(session, "post", "forward", {})
	return err
end

--- Reload the page.
M.refresh = function(session)
	local _, err = M.session_cmd(session, "post", "refresh", {})
	return err
end

--- Get the current URL.
--- @return string? url, string? err
M.get_current_url = function(session)
	local res, err = M.session_cmd(session, "get", "url", {})
	return res and res.value or nil, err
end

--- Get the page title.
--- @return string? title, string? err
M.get_title = function(session)
	local res, err = M.session_cmd(session, "get", "title", {})
	return res and res.value or nil, err
end

--- Find the first element matching a selector.
--- @param session session
--- @param using string   e.g. "css selector", "xpath"
--- @param value string
---
--- @return string? element_id, string? err
M.find_element = function(session, using, value)
	local res, err = M.session_cmd(session, "post", "element", {
		using = using,
		value = value,
	})
	if not res then
		return nil, err
	end
	return res.value.ELEMENT, nil
end

--- Find *all* elements matching a selector.
--- @param session session
--- @param using string   e.g. "css selector", "xpath"
--- @param value string
---
--- @return string[]? element_ids, string? err
M.find_elements = function(session, using, value)
	local res, err = M.session_cmd(session, "post", "elements", {
		using = using,
		value = value,
	})
	if not res then
		return nil, err
	end
	local out = {}
	for i, entry in ipairs(res.value) do
		out[i] = entry.ELEMENT
	end
	return out, nil
end

--- Click on an element.
---
--- @param session session
--- @param element_id string
--- @return string? err
M.click = function(session, element_id)
	local _, err = M.session_cmd(session, "post", ("element/%s/click"):format(element_id), {})
	return err
end

--- Send keystrokes to an element.
---
--- @param session session
--- @param element_id string
--- @param text string
---
--- @return string? err
M.send_keys = function(session, element_id, text)
	-- W3C needs an array of characters:
	local chars = {}
	for i = 1, #text do
		chars[i] = text:sub(i, i)
	end

	local _, err = M.session_cmd(session, "post", ("element/%s/value"):format(element_id), { value = chars })
	return err
end

--- Retrieve the visible text of an element.
---
--- @param session session
--- @param element_id string
---
--- @return string? text, string? err
M.get_text = function(session, element_id)
	local res, err = M.session_cmd(session, "get", ("element/%s/text"):format(element_id), {})
	return res and res.value or nil, err
end

--- Execute synchronous JavaScript in the page.
---
--- @param session session
--- @param script string JS source
--- @param args table? array of arguments
---
--- @return any? result, string? err
M.execute = function(session, script, args)
	local res, err = M.session_cmd(session, "post", "execute/sync", { script = script, args = args or {} })
	return res and res.value or nil, err
end

--- Take a full-page screenshot.
---
--- @param session session
---
--- @return string? base64_png, string? err
M.screenshot = function(session)
	local res, err = M.session_cmd(session, "get", "screenshot", {})
	return res and res.value or nil, err
end

--- Low level helper
---
--- @param session session
--- @param method api_method
--- @param endpoint string
--- @param payload table
---
--- @return table? response, string? error
M.session_cmd = function(session, method, endpoint, payload)
	return wd:session_cmd(session, method, endpoint, payload)
end

--- End webdriver session
---
--- @param session session
---
--- @return string? error
M.session_end = function(session)
	return wd:session_end(session)
end

return M
