local taf = require("taf")
local json = taf.json

taf.test({
	name = "json serialization",
	body = function()
		print("var1: " .. taf.get_var("var1"))
		print("var2: " .. taf.get_var("var2"))
		print("var3: " .. taf.get_var("var3"))
		local test_obj = {
			testint = 1,
			teststr = "test",
			testfloat = 3.14,
			testbool = true,
			testnil = nil,
			testobj = {
				testint = 1,
				teststr = "test",
				testfloat = 3.14,
				testbool = true,
				testnil = nil,
			},
		}

		local json_str = json.serialize(test_obj, { spaced = true, pretty = true })
		print(json_str)

		local test_obj_copy = json.deserialize(json_str)

		assert(test_obj.testint == test_obj_copy.testint)
		assert(test_obj.teststr == test_obj_copy.teststr)
		assert(test_obj.testfloat == test_obj_copy.testfloat)
		assert(test_obj.testbool == test_obj_copy.testbool)
		assert(test_obj.testnil == test_obj_copy.testnil)
		assert(test_obj.testobj.testint == test_obj_copy.testobj.testint)
		assert(test_obj.testobj.teststr == test_obj_copy.testobj.teststr)
		assert(test_obj.testobj.testfloat == test_obj_copy.testobj.testfloat)
		assert(test_obj.testobj.testbool == test_obj_copy.testobj.testbool)
		assert(test_obj.testobj.testnil == test_obj_copy.testobj.testnil)
	end,
})
