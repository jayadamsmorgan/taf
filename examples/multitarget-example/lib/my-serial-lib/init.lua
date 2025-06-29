local M = {}

local taf = require("taf")
local keyword_impl = require("my-serial-" .. taf.get_current_target())

M.keyword1 = function(str1, str2)
	keyword_impl.keyword1(str1, str2)
end

return M
