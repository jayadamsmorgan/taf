local tm = require("taf-main")
local proc = require("taf.proc")
local json = require("taf.json")
local http = require("taf.http")

local M = {}

--- @alias webdriver
--- | '"chromedriver"'
--- | '"geckodriver"'
--- | '"safaridriver"'
--- | '"msedgedriver"'
--- | '"iedriver"'
--- | '"operadriver"'
--- | '"webkitdriver"'
--- | '"wpedriver"'

--- @alias api_method
--- | '"PUT"'
--- | '"POST"'
--- | '"DELETE"'
--- | '"GET"'

--- @class wd_session_opts
--- @field port integer
--- @field url string? webdriver url (optional, defaults to `http://localhost`)

--- @class session
--- @field base_url string
--- @field port integer
--- @field id string

--- Spawn webdriver instance
---
--- @param webdriver string|webdriver path or name of the webdriver executable
--- @param port integer port to open webdriver with
--- @param extraflags [string]? optional array of additional arguments passed to webdriver
---
--- @return proc_handle process handle for the spawned webdriver
M.spawn_webdriver = function(webdriver, port, extraflags)
	local args = extraflags or {}
	table.insert(args, "--port=" .. port)
	local handle = proc.spawn({
		exe = webdriver,
		args = args,
	})

	return handle
end

--- @param url string
--- @param body string
---
--- @return string result
local wd_post_json = function(url, body)
	local result = ""

	local handle = http.new()
	tm:defer(function()
		handle:cleanup()
	end)
	handle
		:setopt(http.OPT_URL, url)
		:setopt(http.OPT_POSTFIELDS, body)
		:setopt(http.OPT_HTTPHEADER, { "Content-Type: application/json" })
		:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
			result = result .. chunk
			return n
		end)

	handle:perform()

	return result
end

--- @param url string
--- @param body string
---
--- @return string result
local wd_put_json = function(url, body)
	local result = ""

	local handle = http.new()
	tm:defer(function()
		handle:cleanup()
	end)
	handle
		:setopt(http.OPT_URL, url)
		:setopt(http.OPT_POSTFIELDS, body)
		:setopt(http.OPT_CUSTOMREQUEST, "PUT")
		:setopt(http.OPT_HTTPHEADER, { "Content-Type: application/json" })
		:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
			result = result .. chunk
			return n
		end)

	handle:perform()

	return result
end

--- @param url string
---
--- @return string result
local wd_get_json = function(url)
	local result = ""

	local handle = http.new()
	tm:defer(function()
		handle:cleanup()
	end)
	handle:setopt(http.OPT_URL, url):setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
		result = result .. chunk
		return n
	end)

	handle:perform()

	return result
end

--- @param url string
---
--- @return string result
local wd_delete_json = function(url)
	local result = ""

	local handle = http.new()
	tm:defer(function()
		handle:cleanup()
	end)
	handle
		:setopt(http.OPT_URL, url)
		:setopt(http.OPT_CUSTOMREQUEST, "DELETE")
		:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
			result = result .. chunk
			return n
		end)
	handle:perform()

	return result
end

--- Start webdriver session
---
--- @param opts wd_session_opts opts to open session with
---
--- @return session
M.session_start = function(opts)
	opts.url = opts.url or "http://localhost"

	local url = opts.url .. ":" .. opts.port .. "/session"
	local body_obj = {
		capabilities = {
			firstMatch = { {} },
		},
		desiredCapabilities = {},
	}
	local body = json.serialize(body_obj)

	local result = wd_post_json(url, body)
	if result == "" then
		error("Unable to start a session: empty result from server")
	end

	local result_obj = json.deserialize(result)

	if not result_obj.value then
		error("Unable to start a session: no value `property` in result object.")
	end

	if result_obj.value.error and result_obj.value.message then
		error("Unable to start a session: " .. result_obj.value.message)
	end

	if not result_obj.value.sessionId then
		error("Unable to start a session: sessionId is not present")
	end

	return {
		base_url = opts.url,
		port = opts.port,
		id = result_obj.value.sessionId,
	}
end

--- @param session session
--- @param url string
---
--- @return table result
M.open_url = function(session, url)
	local payload = {
		url = url,
	}
	local res = M.session_cmd(session, "POST", "url", payload)
	return res
end

--- Go back in history.
--- @param session session
---
--- @return table result
M.go_back = function(session)
	local res = M.session_cmd(session, "POST", "back", {})
	return res
end

--- Go forward in history.
--- @param session session
---
--- @return table result
M.go_forward = function(session)
	local res = M.session_cmd(session, "POST", "forward", {})
	return res
end

--- Reload the page.
--- @param session session
---
--- @return table result
M.refresh = function(session)
	local res = M.session_cmd(session, "POST", "refresh", {})
	return res
end

--- Get the current URL.
--- @param session session
---
--- @return table result
M.get_current_url = function(session)
	local res = M.session_cmd(session, "GET", "url", {})
	return res.value
end

--- Get the page title.
---
--- @return string title
M.get_title = function(session)
	local res = M.session_cmd(session, "GET", "title", {})
	return res.value
end

--- Find the first element matching a selector.
---
--- @param session session
--- @param using string   e.g. "css selector", "xpath"
--- @param value string
---
--- @return string element_id
M.find_element = function(session, using, value)
	local res = M.session_cmd(session, "POST", "element", {
		using = using,
		value = value,
	})
	return res.value.ELEMENT
end

--- Find *all* elements matching a selector.
--- @param session session
--- @param using string   e.g. "css selector", "xpath"
--- @param value string
---
--- @return string[] element_ids
M.find_elements = function(session, using, value)
	local res = M.session_cmd(session, "POST", "elements", {
		using = using,
		value = value,
	})
	local out = {}
	for i, entry in ipairs(res.value) do
		out[i] = entry.ELEMENT
	end
	return out
end

--- Click on an element.
---
--- @param session session
--- @param element_id string
--- @return table result
M.click = function(session, element_id)
	local res = M.session_cmd(session, "POST", ("element/%s/click"):format(element_id), {})
	return res
end

--- Send keystrokes to an element.
---
--- @param session session
--- @param element_id string
--- @param text string
---
--- @return table result
M.send_keys = function(session, element_id, text)
	-- W3C needs an array of characters:
	local chars = {}
	for i = 1, #text do
		chars[i] = text:sub(i, i)
	end

	local res = M.session_cmd(session, "POST", ("element/%s/value"):format(element_id), { value = chars })
	return res
end

--- Retrieve the visible text of an element.
---
--- @param session session
--- @param element_id string
---
--- @return string text
M.get_text = function(session, element_id)
	local res = M.session_cmd(session, "GET", ("element/%s/text"):format(element_id), {})
	return res.value
end

--- Execute synchronous JavaScript in the page.
---
--- @param session session
--- @param script string JS source
--- @param args table? array of arguments
---
--- @return table result
M.execute = function(session, script, args)
	local res = M.session_cmd(session, "POST", "execute/sync", { script = script, args = args or {} })
	return res
end

--- Take a full-page screenshot.
---
--- @param session session
---
--- @return string? base64_png
M.screenshot = function(session)
	local res = M.session_cmd(session, "GET", "screenshot", {})
	return res.value
end

--- Low level helper
---
--- @param session session
--- @param method api_method
--- @param endpoint string
--- @param payload table? payload if method is "post" or "put"
---
--- @return table response
M.session_cmd = function(session, method, endpoint, payload)
	payload = payload or {}
	local url = session.base_url .. ":" .. session.port .. "/session/" .. session.id .. "/" .. endpoint
	local body = json.serialize(payload)
	if method == "PUT" or method == "put" then
		local result = wd_put_json(url, body)
		return json.deserialize(result)
	end
	if method == "POST" or method == "post" then
		local result = wd_post_json(url, body)
		return json.deserialize(result)
	end
	if method == "DELETE" or method == "delete" then
		local result = wd_delete_json(url)
		return json.deserialize(result)
	end
	if method == "GET" or method == "get" then
		local result = wd_get_json(url)
		return json.deserialize(result)
	end

	error("Unknown request " .. method)
end

--- End webdriver session
---
--- @param session session
M.session_end = function(session)
	local url = session.base_url .. ":" .. session.port .. "/session/" .. session.id
	wd_delete_json(url)
end

return M
