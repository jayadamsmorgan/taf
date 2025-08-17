local taf = require("taf")
local serial = taf.serial
local vars = require("variables")

taf.test({
	name = "List serial devices",
	tags = { "tag1", "tag2" },
	body = function()
		local devices = serial.list_devices()
		for i, device in ipairs(devices) do
			print("[" .. i .. "]: Found device: ", device.path, device.type, device.description)
		end
	end,
})

taf.test({
	name = "Get port info",
	tags = { "tag2", "tag3" },
	body = function()
		local port = serial.get_port(vars.port_name)
		local info = port:get_port_info()
		print("Found device: ", info.path, info.type, info.description)
	end,
})

taf.test({
	name = "Reading from serial device",
	tags = { "tag2" },
	body = function()
		local port = serial.get_port(vars.port_name)

		port:open("rw")

		taf.defer(function()
			port:close()
		end)

		port:set_baudrate(vars.baudrate)
		port:set_bits(vars.bits)
		port:set_parity(vars.parity)
		port:set_stopbits(vars.stopbits)

		local result = port:read_until("Hello Rockchip", 15000)
		print(result)
	end,
})
