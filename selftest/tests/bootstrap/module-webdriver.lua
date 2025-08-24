local taf = require("taf")
local wd = taf.webdriver

taf.test({
	name = "Test minimal webdriver possibilities",
	tags = { "module-webdriver" },
	body = function()
		local port = 9515
		local proc_handle = wd.spawn_webdriver({
			webdriver = "chromedriver",
			port = port,
		})
		taf.defer(function()
			proc_handle:kill()
		end)
		taf.sleep(5000) -- wait for the webdriver to start just to make sure
		local session = wd.session_start({
			port = port,
			headless = false,
		})
		taf.defer(function()
			wd.session_end(session)
		end)
		wd.open_url(session, "https://github.com/")
		taf.log_info(wd.get_current_url(session))
		taf.log_info(wd.get_title(session))
	end,
})
