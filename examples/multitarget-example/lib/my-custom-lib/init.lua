local M = {}

local taf = require("taf")
local target = taf.get_current_target()
local keyword_impl = require("my-custom-lib." .. target)

M.keyword1 = function(str1, str2)
	print("Test target: " .. target)
	return keyword_impl.keyword1(str1, str2)
end

M.keyword2 = function(str1, str2)
	print("Test target: " .. target)
	return keyword_impl.keyword2(str1, str2)
end

return M
