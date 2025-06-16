local taf = require("taf")
local serial = taf.serial
local vars = require("variables")

taf.test("List serial devices", { "tag1", "tag2" }, function()
	local devices = serial.list_devices()
	for i, device in ipairs(devices) do
		print("[" .. i .. "]: Found device: ", device.path, device.type, device.description)
	end
end)

taf.test("Get port info", { "tag2", "tag3" }, function()
	local port = serial.get_port(vars.port_name)
	local info = port:get_port_info()
	print("Found device: ", info.path, info.type, info.description)
end)

taf.test("Reading from serial device", { "tag2" }, function()
	local port = serial.get_port(vars.port_name)

	port:open("rw")

	taf.defer(function()
		port:close()
	end)

	port:set_baudrate(vars.baudrate)
	port:set_bits(vars.bits)
	port:set_parity(vars.parity)
	port:set_stopbits(vars.stopbits)

	local result = port:read_until("src.-CH8", 4000)
	print(result)
end)
