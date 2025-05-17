local taf = require("taf")
local ts = require("taf-serial")

local M = {}

--- C pointer to serial port handle
---
--- @alias SerialPort userdata

--- Mode to open serial device with
---
--- @alias SerialMode
--- | '"r"'  Read
--- | '"w"'  Write
--- | '"rw"' Read & Write

--- Amount of data bits in serial communication
---
--- @alias SerialDataBits
--- | 5
--- | 6
--- | 7
--- | 8

--- Parity of serial communication
---
--- @alias SerialParity
--- | '"none"'
--- | '"odd"'
--- | '"even"'
--- | '"mark"'
--- | '"space"'

--- Amount of stop bits in serial communication
---
--- @alias SerialStopBits
--- | 1
--- | 2

--- Options to open serial port with
---
--- @class SerialOpts
--- @field mode SerialMode?
--- @field baudrate number?
--- @field bits SerialDataBits?
--- @field parity SerialParity?
--- @field stopbits SerialStopBits?

--- Type of the serial device connected
---
--- @alias SerialPortType
--- | '"native"'
--- | '"usb"'
--- | '"bluetooth"'
--- | '"unknown"'

--- Full info about the serial port
---
--- @class SerialPortInfo
--- @field path string path or name of the serial port
--- @field type SerialPortType type of serial port
--- @field description string description of the serial device
--- @field serial string? serial number of usb serial device
--- @field product string? product string of usb serial device
--- @field manufacturer string? manufacturer string of usb serial device
--- @field vid number? vendor id of usb serial device
--- @field pid number? product id of usb serial device
--- @field usb_address number? usb port address of usb serial device
--- @field usb_bus number? usb bus number of usb serial device
--- @field bluetooth_address string? bluetooth MAC address of bluetooth serial device

--- @type SerialOpts
local default_serial_opts = {
	mode = "rw",
	baudrate = 115200,
	bits = 8,
	parity = "none",
	stopbits = 1,
}

--- Default options for serial.open()
---
--- @return SerialOpts opts
M.default_serial_options = function()
	return default_serial_opts
end

--- Open serial device port with options
---
--- @param port_or_path SerialPort | string path to serial device or serial port to open
--- @param opts SerialOpts? options to open serial device with
---
--- @return SerialPort? result, string? error
M.open = function(port_or_path, opts)
	--- @type SerialPort?
	local port

	--- @type string?
	local err

	if type(port_or_path) == "string" then
		port, err = ts:get_port(port_or_path)
		if err then
			return nil, err
		end
	elseif type(port_or_path) == "SerialPort" then
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
--- @param port SerialPort port returned after serial.open()
---
--- @return string? error
M.close = function(port)
	return ts:close(port)
end

--- Set baudrate for opened port
---
--- @param port SerialPort port returned after serial.open()
--- @param baudrate number
---
--- @return string? error
M.set_baudrate = function(port, baudrate)
	return ts:set_baudrate(port, baudrate)
end

--- Set data bits for opened port
---
--- @param port SerialPort port returned after serial.open()
--- @param bits SerialDataBits
---
--- @return string? error
M.set_bits = function(port, bits)
	return ts:set_bits(port, bits)
end

--- Set parity for opened port
---
--- @param port SerialPort port returned after serial.open()
--- @param parity SerialParity
---
--- @return string? error
M.set_parity = function(port, parity)
	return ts:set_parity(port, parity)
end

--- Set stopbits for opened port
---
--- @param port SerialPort port returned after serial.open()
--- @param stopbits SerialStopBits
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
--- @param port SerialPort
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
--- @param port SerialPort
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
--- @param port SerialPort
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
--- @param port SerialPort
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
---@param port SerialPort
---@param xonxoff SerialXONXOFF
---
---@return string? error
M.set_xonxoff = function(port, xonxoff)
	return ts:set_xonxoff(port, xonxoff)
end

--- Serial RS232 flowcontrol option
---
--- @alias SerialFlowCtrl
--- | '"dtrdsr"'
--- | '"rtscts"'
--- | '"xonxoff"'
--- | '"none"'

--- Set RS232 flowcontrol option for opened port
---
--- @param port SerialPort
--- @param flowctrl SerialFlowCtrl
---
--- @return string? error
M.set_flowcontrol = function(port, flowctrl)
	return ts:set_flowcontrol(port, flowctrl)
end

--- List info about all connected serial devices in the system
---
--- @return SerialPortInfo[]? result, string? error
M.list_devices = function()
	return ts:list_devices()
end

--- @param port_or_path SerialPort | string
---
--- @return SerialPortInfo? result, string? error
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
--- @param port SerialPort
--- @param pattern string? the pattern to look for, defaults to newline
--- @param timeout number? optional timeout value. keep nil for blocking indefinetely, 0 for nonblocking reads
--- @param chunk_size number? the size of the read chunks, defaults to 1
---
--- @return string result, string? error
M.read_until = function(port, pattern, timeout, chunk_size)
	local now_ms = function()
		return taf:millis()
	end
	chunk_size = chunk_size or 1
	pattern = pattern or "\n" -- default to newline
	local buf = {} -- table buffer → concat later
	local deadline = nil
	if timeout and timeout > 0 then
		deadline = now_ms() + timeout -- you need a millisecond helper
	end

	while true do
		local remaining = 0
		if deadline then
			remaining = math.max(0, math.ceil(deadline - now_ms()))
			if remaining == 0 then
				return "", "timeout"
			end
		end

		local chunk, err
		if timeout == nil then
			-- block indefinitely
			chunk, err = ts:read_blocking(port, chunk_size, 0) -- 0 = forever
		elseif timeout == 0 then
			-- non-blocking; returns immediately if no data in buffer
			chunk, err = ts:read_nonblocking(port, chunk_size)
			if not chunk or #chunk == 0 then
				return "", err or "would block"
			end
		else
			-- finite timeout
			chunk, err = ts:read_blocking(port, chunk_size, remaining)
			if not chunk then
				return "", err
			end
			if #chunk == 0 then
				return "", "timeout"
			end
		end

		buf[#buf + 1] = chunk
		local current = table.concat(buf)

		if current:find(pattern) then
			return current, nil
		end
	end
end

return M
