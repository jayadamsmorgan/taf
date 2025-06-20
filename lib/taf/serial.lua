local taf = require("taf-main")
local ts = require("taf-serial")

local M = {}

M.low = ts

--- @alias serial_flush_direction
--- | '"i"' input
--- | '"o"' output
--- | '"io"' input/ouput

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

--- Type of the serial device connected
---
--- @alias serial_port_type
--- | '"native"'
--- | '"usb"'
--- | '"bluetooth"'
--- | '"unknown"'

--- Serial RS232 DSR option
---
--- @alias serial_dsr
--- | '"ignore"'
--- | '"flowctrl"'

--- Serial RS232 CTS option
---
--- @alias serial_cts
--- | '"ignore"'
--- | '"flowctrl"'

--- Serial RS232 DTR option
---
--- @alias serial_dtr
--- | '"off"'
--- | '"on"'
--- | '"flowctrl"'

--- Serial RS232 RTS option
---
--- @alias serial_rts
--- | '"off"'
--- | '"on"'
--- | '"flowctrl"'

--- Serial RS232 XON/XOFF option
---
--- @alias serial_xonxoff
--- | '"i"'       input
--- | '"o"'       output
--- | '"io"'      input/output
--- | '"disable"' disable XON/XOFF

--- Serial RS232 flowcontrol option
---
--- @alias serial_flowctrl
--- | '"dtrdsr"'
--- | '"rtscts"'
--- | '"xonxoff"'
--- | '"none"'

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

--- @alias close_func fun(self:serial_port)
--- @alias drain_func fun(self:serial_port)
--- @alias flush_func fun(self:serial_port, direction:serial_flush_direction)
--- @alias get_port_info_func fun(self:serial_port):serial_port_info
--- @alias get_waiting_input_func fun(self:serial_port): integer
--- @alias get_waiting_output_func fun(self:serial_port): integer
--- @alias open_func fun(self:serial_port, mode: serial_mode)
--- @alias read_blocking_func fun(self:serial_port, byte_amount:integer, timeout:integer?): string
--- @alias read_nonblocking_func fun(self:serial_port, byte_amount:integer): string
--- @alias read_until_func fun(self:serial_port, pattern:string?, timeout:number?, chunk_size:number?): string
--- @alias set_baudrate_func fun(self:serial_port, baudrate:integer)
--- @alias set_bits_func fun(self:serial_port, bits:serial_data_bits)
--- @alias set_cts_func fun(self:serial_port, cts:serial_cts)
--- @alias set_dsr_func fun(self:serial_port, dsr:serial_dsr)
--- @alias set_dtr_func fun(self:serial_port, dtr:serial_dtr)
--- @alias set_flowcontrol_func fun(self:serial_port, flowctrl:serial_flowctrl)
--- @alias set_parity_func fun(self:serial_port, parity:serial_parity)
--- @alias set_rts_func fun(self:serial_port, rts:serial_rts)
--- @alias set_stopbits_func fun(self:serial_port, stopbits:serial_stop_bits)
--- @alias set_xon_xoff_func fun(self:serial_port, xonxoff:serial_xonxoff)
--- @alias write_blocking_func fun(self:serial_port, str:string, timeout:integer): integer
--- @alias write_nonblocking_func fun(self:serial_port, str:string): integer

--- Serial port handle
---
--- @class serial_port
--- @field close close_func
--- @field drain drain_func
--- @field flush flush_func
--- @field get_port_info get_port_info_func
--- @field get_waiting_input get_waiting_input_func
--- @field get_waiting_output get_waiting_output_func
--- @field open open_func
--- @field read_blocking read_blocking_func
--- @field read_nonblocking read_nonblocking_func
--- @field read_until read_until_func
--- @field set_baudrate set_baudrate_func
--- @field set_bits set_bits_func
--- @field set_cts set_cts_func
--- @field set_dsr set_dsr_func
--- @field set_dtr set_dtr_func
--- @field set_flowcontrol set_flowcontrol_func
--- @field set_parity set_parity_func
--- @field set_rts set_rts_func
--- @field set_stopbits set_stopbits_func
--- @field set_xon_xoff set_xon_xoff_func
--- @field private write_blocking write_blocking_func
--- @field private write_nonblocking write_nonblocking_func

--- Get port by it's path or name (but not open it)
---
--- @param path string
---
--- @return serial_port
M.get_port = function(path)
	local port = ts:get_port(path)
	local mt = getmetatable(port)
	function mt.__index:read_until(pattern, timeout, chunk_size)
		return M.read_until(self, pattern, timeout, chunk_size)
	end
	return port
end

--- List info about all connected serial devices in the system
---
--- @return serial_port_info[] result
M.list_devices = function()
	return ts:list_devices()
end

--- Reads from serial port until it encounters the matching pattern
--- Returns full string read if pattern occured, empty string otherwise
---
--- @param port serial_port
--- @param pattern string? the pattern to look for, defaults to newline
--- @param timeout number? optional timeout value. keep nil for blocking indefinetely, 0 for nonblocking reads
--- @param chunk_size number? the size of the read chunks, defaults to 64
---
--- @return string result
M.read_until = function(port, pattern, timeout, chunk_size)
	port:flush("io")

	local now_ms = function()
		return taf:millis()
	end

	chunk_size = chunk_size or 64
	pattern = pattern or "\n"

	local buftable = {}
	local buflen = 0
	local deadline
	if timeout and timeout > 0 then
		deadline = now_ms() + timeout
	end

	while true do
		if deadline then
			local remaining = math.max(0, math.ceil(deadline - now_ms()))
			if remaining == 0 then
				error("timeout")
			end
		end

		local chunk
		if timeout == nil then
			chunk = port:read_blocking(chunk_size, 0)
		elseif timeout == 0 then
			chunk = port:read_nonblocking(chunk_size)
		else
			local remaining = math.max(1, math.ceil(deadline - now_ms()))
			chunk = port:read_blocking(chunk_size, remaining)
		end

		if chunk and #chunk > 0 then
			table.insert(buftable, chunk)
			buflen = buflen + #chunk

			local full_buf = table.concat(buftable)
			if full_buf:find(pattern, 1, true) then
				return full_buf
			end
		end

		if timeout == 0 and (not chunk or #chunk == 0) then
			break
		end
	end

	return ""
end

return M
