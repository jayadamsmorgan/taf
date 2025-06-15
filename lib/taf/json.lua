local json = require("taf-json")

local M = {}

M.low = json

--- @class json_serialize_opts
--- @field spaced boolean? add spaces whereever necessary (`false` by default)
--- @field pretty boolean? format "pretty" (indent: 2 spaces) (`false` by default)
--- @field pretty_tab boolean? use tabs instead of 2 spaces if `pretty` (`false` by default)
--- @field no_trailing_zero boolean? to remove or not trailing zero on float values (`false` by default)
--- @field slash_escape boolean? to escape or not backslashes for C string (`false` by default)
--- @field color boolean? makes json string with terminal escape codes with different colors (`false` by default)

--- Serialize Lua table into string JSON
--- @param object any
--- @param opts json_serialize_opts?
---
--- @return string json_string
M.serialize = function(object, opts)
	local ser_opts = opts or {}
	ser_opts.spaced = ser_opts.spaced or false
	ser_opts.pretty = ser_opts.pretty or false
	ser_opts.pretty_tab = ser_opts.pretty_tab or false
	ser_opts.no_trailing_zero = ser_opts.no_trailing_zero or false
	ser_opts.slash_escape = ser_opts.slash_escape or false
	ser_opts.color = ser_opts.color or false

	local flag = 0
	if ser_opts.spaced == true then
		flag = 1
	end
	if ser_opts.pretty == true then
		flag = flag + 2
	end
	if ser_opts.pretty_tab == true then
		flag = flag + 8
	end
	if ser_opts.no_trailing_zero == true then
		flag = flag + 4
	end
	if ser_opts.slash_escape == false then -- slash escape is on by default in json-c, inverting here
		flag = flag + 16
	end
	if ser_opts.color == true then
		flag = flag + 32
	end

	return json:serialize(object, flag)
end

--- Deserialize JSON string into Lua table
--- @param str string
---
--- @return table object
M.deserialize = function(str)
	return json:deserialize(str)
end

return M
