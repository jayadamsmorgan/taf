local M = {}

--- @param s string
--- @return boolean
M.is_valid_datetime = function(s)
	local MM, DD, YY, hh, mm, ss = s:match("^(%d%d)%.(%d%d)%.(%d%d)%-(%d%d):(%d%d):(%d%d)$")
	if not MM then
		return false
	end

	MM, DD, YY, hh, mm, ss = tonumber(MM), tonumber(DD), tonumber(YY), tonumber(hh), tonumber(mm), tonumber(ss)

	if MM < 1 or MM > 12 then
		return false
	end
	if hh > 23 or mm > 59 or ss > 59 then
		return false
	end

	local dim = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	if YY % 4 == 0 then
		dim[2] = 29
	end

	if DD < 1 or DD > dim[MM] then
		return false
	end

	return true
end

return M
