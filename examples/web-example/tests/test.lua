local taf = require("taf")
local web = taf.web

taf.test("testing web session start", function()
	-- Open the session on port 9515
	local session, err = web.session_start(9515)
	assert(not err, err or "")

	taf.sleep(1000)

	-- Open google.com
	err = web.open_url(session, "https://google.com/")
	assert(not err, err or "")

	taf.sleep(1000)

	-- Open yahoo.com
	err = web.open_url(session, "https://yahoo.com/")
	assert(not err, err or "")

	taf.sleep(1000)

	-- Go back to google.com
	err = web.go_back(session)

	taf.sleep(1000)

	-- Go forward to yahoo.com
	err = web.go_forward(session)

	taf.sleep(1000)

	-- Refresh the site
	err = web.refresh(session)

	taf.sleep(1000)

	-- Get the site url
	local url
	url, err = web.get_current_url(session)
	assert(not err and url, err or "url is nil")
	taf.print("Example")
	assert(url == "https://www.yahoo.com/")

	-- Get the site title
	local title
	title, err = web.get_title(session)
	assert(not err and title, err or "title is nil")
	assert(title == "Yahoo | Mail, Weather, Search, Politics, News, Finance, Sports & Videos")

	taf.sleep(500)

	-- Close webdriver session
	err = web.session_end(session)
	assert(not err, err or "")
end)
