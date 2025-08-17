local taf = require("taf")

taf.test({
	name = "Test common TAF test",
	tags = { "common" },
	body = function()
		taf.log_info("common")
	end,
})
