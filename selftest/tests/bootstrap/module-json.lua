local taf = require("taf")
local json = taf.json

taf.test({
	name = "Test JSON serialization",
	tags = { "module-json" },
	body = function()
		local test_object = {
			integer = 2,
			double = 3.14,
			another_object = {
				string = "test_string",
			},
			int_array = { 1, 2, 3, 4 },
			str_array = { "1", "2", "3", "4" },
			map = {
				["1"] = 1,
				["2"] = 2,
				["3"] = 3,
				["4"] = 4,
			},
		}

		local str = json.serialize(test_object)
		assert(str)
		taf.log_info(str)
	end,
})

taf.test({
	name = "Test JSON deserialization with good string",
	tags = { "module-json" },
	body = function()
		local str = [[
    {
        "integer": 2,
        "double": 3.14,
        "another_object": {
            "string": "test_string"
        },
        "int_array": [1, 2, 3, 4],
        "str_array": ["1", "2", "3", "4"],
    }
    ]]

		local test_object = json.deserialize(str)
		taf.log_info(test_object.integer)
		taf.log_info(test_object.double)
		taf.log_info(test_object.another_object.string)
		local total = 0
		for _, value in ipairs(test_object.int_array) do
			total = total + value
		end
		taf.log_info(total)
		local value_str = ""
		for _, value in ipairs(test_object.str_array) do
			value_str = value_str .. value
		end
		taf.log_info(value_str)
	end,
})

taf.test({
	name = "Test JSON deserialization with bad string",
	tags = { "module-json" },
	body = function()
		local str = "incorrect"

		-- Should throw
		local test_object = json.deserialize(str)
		taf.log_info(test_object)
	end,
})
