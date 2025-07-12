local taf = require("taf")
local wd = taf.webdriver

taf.test("Test minimal webdriver possibilities", { "module-webdriver" }, function()
	local proc_handle = wd.spawn_webdriver("chromedriver", 9515)
	taf.defer(function()
		proc_handle:kill()
	end)
	taf.sleep(100) -- wait for the webdriver to start just to make sure
	local session = wd.session_start({ port = 9515, headless = "chromedriver" })
	taf.defer(function()
		wd.session_end(session)
	end)
	wd.open_url(session, "https://github.com/")
	taf.log_info(wd.get_current_url(session))
	taf.log_info(wd.get_title(session))
end)
