local keywords = require("keywords")

test_case("LED blinks 1 times", { "serial" }, function()
	keywords.custom_keyword()

	test.sleep(100)
	test.sleep(100)
end)
