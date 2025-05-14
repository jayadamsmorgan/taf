local hs = require("herness-serial")

local M = {}

---@class SerialOpts
---@field mode string?
---@field baudrate number?
---@field bits number?
---@field parity string?
---@field stopbits number?

---@type SerialOpts
local default_serial_opts = {
	mode = "rw",
	baudrate = 115200,
	bits = 8,
	parity = "none",
	stopbits = 1,
}

---@param path string path to serial device to open
---@param opts SerialOpts? options to open serial device with
---
---@return any?, string?
M.open = function(path, opts)
	if not path then
		return nil, "path is nil"
	end

	if not opts then
		opts = default_serial_opts
	else
		opts.mode = opts.mode or default_serial_opts.mode
		opts.baudrate = opts.baudrate or default_serial_opts.baudrate
		opts.bits = opts.bits or default_serial_opts.bits
		opts.parity = opts.parity or default_serial_opts.parity
		opts.stopbits = opts.stopbits or default_serial_opts.stopbits
	end

	local port, err = hs:get_port(path)
	if err then
		return nil, err
	end

	err = hs:open(port, opts.mode)
	if err then
		return nil, err
	end

	err = hs:set_baudrate(port, opts.baudrate)
	if err then
		hs:close(port)
		return nil, err
	end

	err = hs:set_bits(port, opts.bits)
	if err then
		hs:close(port)
		return nil, err
	end

	err = hs:set_parity(port, opts.parity)
	if err then
		hs:close(port)
		return nil, err
	end

	err = hs:set_stopbits(port, opts.stopbits)
	if err then
		hs:close(port)
		return nil, err
	end

	return port, nil
end

M.close = function() end

return M
