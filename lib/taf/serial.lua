local taf = require("taf-main")
local ts = require("taf-serial")

local M = {}

M.low = ts

--- C pointer to serial port handle
---
--- @alias serial_port userdata

--- Mode to open serial device with
---
--- @alias serial_mode
--- | '"r"'  Read
--- | '"w"'  Write
--- | '"rw"' Read & Write

--- Amount of data bits in serial communication
---
--- @alias serial_data_bits
--- | 5
--- | 6
--- | 7
--- | 8

--- Parity of serial communication
---
--- @alias serial_parity
--- | '"none"'
--- | '"odd"'
--- | '"even"'
--- | '"mark"'
--- | '"space"'

--- Amount of stop bits in serial communication
---
--- @alias serial_stop_bits
--- | 1
--- | 2

--- Options to open serial port with
---
--- @class serial_opts
--- @field mode serial_mode?
--- @field baudrate number?
--- @field bits serial_data_bits?
--- @field parity serial_parity?
--- @field stopbits serial_stop_bits?

--- Type of the serial device connected
---
--- @alias serial_port_type
--- | '"native"'
--- | '"usb"'
--- | '"bluetooth"'
--- | '"unknown"'

--- Full info about the serial port
---
--- @class serial_port_info
--- @field path string path or name of the serial port
--- @field type serial_port_type type of serial port
--- @field description string description of the serial device
--- @field serial string? serial number of usb serial device
--- @field product string? product string of usb serial device
--- @field manufacturer string? manufacturer string of usb serial device
--- @field vid number? vendor id of usb serial device
--- @field pid number? product id of usb serial device
--- @field usb_address number? usb port address of usb serial device
--- @field usb_bus number? usb bus number of usb serial device
--- @field bluetooth_address string? bluetooth MAC address of bluetooth serial device

--- @type serial_opts
local default_serial_opts = {
	mode = "rw",
	baudrate = 115200,
	bits = 8,
	parity = "none",
	stopbits = 1,
}

--- Default options for serial.open()
---
--- @return serial_opts opts
M.default_serial_options = function()
	return default_serial_opts
end

--- Open serial device port with options
---
--- @param port_or_path serial_port | string path to serial device or serial port to open
--- @param opts serial_opts? options to open serial device with
---
--- @return serial_port? result, string? error
M.open = function(port_or_path, opts)
	--- @type serial_port?
	local port

	--- @type string?
	local err

	if type(port_or_path) == "string" then
		port, err = ts:get_port(port_or_path)
		if err then
			return nil, err
		end
	elseif type(port_or_path) == "userdata" then
		port = port_or_path
	else
		return nil, "argument 'port_or_path' is of invalid type or nil"
	end

	local default_opts = default_serial_opts

	if not opts then
		opts = default_opts
	else
		opts.mode = opts.mode or default_opts.mode
		opts.baudrate = opts.baudrate or default_opts.baudrate
		opts.bits = opts.bits or default_opts.bits
		opts.parity = opts.parity or default_opts.parity
		opts.stopbits = opts.stopbits or default_opts.stopbits
	end

	err = ts:open(port, opts.mode)
	if err then
		return nil, err
	end

	err = ts:set_baudrate(port, opts.baudrate)
	if err then
		ts:close(port)
		return nil, err
	end

	err = ts:set_bits(port, opts.bits)
	if err then
		ts:close(port)
		return nil, err
	end

	err = ts:set_parity(port, opts.parity)
	if err then
		ts:close(port)
		return nil, err
	end

	err = ts:set_stopbits(port, opts.stopbits)
	if err then
		ts:close(port)
		return nil, err
	end

	return port, nil
end

--- Close the opened port
---
--- @param port serial_port port returned after serial.open()
---
--- @return string? error
M.close = function(port)
	return ts:close(port)
end

--- Set baudrate for opened port
---
--- @param port serial_port port returned after serial.open()
--- @param baudrate number
---
--- @return string? error
M.set_baudrate = function(port, baudrate)
	return ts:set_baudrate(port, baudrate)
end

--- Set data bits for opened port
---
--- @param port serial_port port returned after serial.open()
--- @param bits serial_data_bits
---
--- @return string? error
M.set_bits = function(port, bits)
	return ts:set_bits(port, bits)
end

--- Set parity for opened port
---
--- @param port serial_port port returned after serial.open()
--- @param parity serial_parity
---
--- @return string? error
M.set_parity = function(port, parity)
	return ts:set_parity(port, parity)
end

--- Set stopbits for opened port
---
--- @param port serial_port port returned after serial.open()
--- @param stopbits serial_stop_bits
---
--- @return string? error
M.set_stopbits = function(port, stopbits)
	return ts:set_stopbits(port, stopbits)
end

--- Serial RS232 RTS option
---
--- @alias SerialRTS
--- | '"off"'
--- | '"on"'
--- | '"flowctrl"'

--- Set RS232 RTS for opened port
---
--- @param port serial_port
--- @param rts SerialRTS
---
--- @return string? error
M.set_rts = function(port, rts)
	return ts:set_rts(port, rts)
end

--- Serial RS232 CTS option
---
--- @alias SerialCTS
--- | '"ignore"'
--- | '"flowctrl"'

--- Set RS232 CTS for opened port
---
--- @param port serial_port
--- @param cts SerialCTS
---
--- @return string? error
M.set_cts = function(port, cts)
	return ts:set_cts(port, cts)
end

--- Serial RS232 DTR option
---
--- @alias SerialDTR
--- | '"off"'
--- | '"on"'
--- | '"flowctrl"'

--- Set RS232 DTR for opened port
---
--- @param port serial_port
--- @param dtr SerialDTR
---
--- @return string? error
M.set_dtr = function(port, dtr)
	return ts:set_dtr(port, dtr)
end

--- Serial RS232 DSR option
---
--- @alias SerialDSR
--- | '"ignore"'
--- | '"flowctrl"'

--- Set RS232 DSR for opened port
---
--- @param port serial_port
--- @param dsr SerialDSR
---
--- @return string? error
M.set_dsr = function(port, dsr)
	return ts:set_dsr(port, dsr)
end

--- Serial RS232 XON/XOFF option
---
--- @alias SerialXONXOFF
--- | '"i"'       input
--- | '"o"'       output
--- | '"io"'      input/output
--- | '"disable"' disable XON/XOFF

--- Set RS232 XON/XOFF for opened port
---
---@param port serial_port
---@param xonxoff SerialXONXOFF
---
---@return string? error
M.set_xonxoff = function(port, xonxoff)
	return ts:set_xonxoff(port, xonxoff)
end

--- Serial RS232 flowcontrol option
---
--- @alias serial_flowctrl
--- | '"dtrdsr"'
--- | '"rtscts"'
--- | '"xonxoff"'
--- | '"none"'

--- Set RS232 flowcontrol option for opened port
---
--- @param port serial_port
--- @param flowctrl serial_flowctrl
---
--- @return string? error
M.set_flowcontrol = function(port, flowctrl)
	return ts:set_flowcontrol(port, flowctrl)
end

--- List info about all connected serial devices in the system
---
--- @return serial_port_info[]? result, string? error
M.list_devices = function()
	return ts:list_devices()
end

--- @param port_or_path serial_port | string
---
--- @return serial_port_info? result, string? error
M.get_port_info = function(port_or_path)
	local port = port_or_path
	if type(port_or_path) == "string" then
		local err
		port, err = ts:get_port(port_or_path)
		if err then
			return nil, err
		end
	end

	return ts:get_port_info(port), nil
end

--- Reads from serial port until it encounters the matching pattern
--- Returns full string read if pattern occured, empty string otherwise
---
--- @param port serial_port
--- @param pattern string? the pattern to look for, defaults to newline
--- @param timeout number? optional timeout value. keep nil for blocking indefinetely, 0 for nonblocking reads
--- @param chunk_size number? the size of the read chunks, defaults to 64
---
--- @return string result, string? error
M.read_until = function(port, pattern, timeout, chunk_size)
	M.flush(port, "io")

	local now_ms = function()
		return taf:millis()
	end

	chunk_size = chunk_size or 64
	pattern = pattern or "\n"

	local buf = {} -- pieces collected so far
	local deadline
	if timeout and timeout > 0 then
		deadline = now_ms() + timeout
	end

	local function collected()
		return table.concat(buf)
	end

	while true do
		if deadline then
			local remaining = math.max(0, math.ceil(deadline - now_ms()))
			if remaining == 0 then
				return collected(), "timeout"
			end
		end

		local chunk, err
		if timeout == nil then
			-- block forever
			chunk, err = ts:read_blocking(port, chunk_size, 0)
		elseif timeout == 0 then
			-- non-blocking
			chunk, err = ts:read_nonblocking(port, chunk_size)
			if not chunk or #chunk == 0 then
				return collected(), err or "would block"
			end
		else
			-- finite timeout
			local remaining = math.max(1, math.ceil(deadline - now_ms()))
			chunk, err = ts:read_blocking(port, chunk_size, remaining)
			if not chunk then
				return collected(), err -- I/O failure
			end
			if #chunk == 0 then
				return collected(), "timeout" -- timed out with no data
			end
		end

		-- got data – stash it
		buf[#buf + 1] = chunk

		-- stop when pattern seen
		if collected():find(pattern) then
			return collected(), nil
		end
	end
end

--- Write string to serial port
---
--- @param port serial_port
--- @param buffer string string to write to serial port
--- @param timeout number? optional timeout for blocking writes. keep nil for blocking indefinetely, 0 for nonblocking writes
---
--- @return number? bytes_written, string? error
M.write = function(port, buffer, timeout)
	if timeout == nil then
		return ts:write_blocking(port, buffer, 0)
	elseif timeout == 0 then
		return ts:write_nonblocking(port, buffer)
	else
		return ts:write_blocking(port, buffer, timeout)
	end
end

--- Get the number of bytes waiting in the input buffer
---
--- @param port serial_port
---
--- @return number? bytes, string? error
M.input_waiting = function(port)
	return ts:input_waiting(port)
end

--- Get the number of bytes waiting in the output buffer
---
--- @param port serial_port
---
--- @return number? bytes, string? error
M.output_waiting = function(port)
	return ts:output_waiting(port)
end

--- @alias serial_flush_direction
--- | '"i"' input
--- | '"o"' output
--- | '"io"' input/ouput

--- Flush serial port buffers
---
--- @param port serial_port
--- @param direction serial_flush_direction
---
--- @return string? error
M.flush = function(port, direction)
	return ts:flush(port, direction)
end

--- Wait for buffered data to be transmitted
---
--- @param port serial_port
---
--- @return string? error
M.drain = function(port)
	return ts:drain(port)
end

--- Get port by it's path or name (but not open it)
---
--- @param path string
---
--- @return serial_port
M.get_port = function(path)
	return ts:get_port(path)
end

return M
