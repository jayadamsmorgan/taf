local taf = require("taf")
local serial = taf.serial
local vars = require("variables")

taf.test("List serial devices", function()
	local devices, err = serial.list_devices()
	if err then
		print(err)
		return
	end
	assert(devices ~= nil)
	for i, device in ipairs(devices) do
		print(i)
		print(device.path)
		print(device.type)
		print(device.description)
	end
end)

taf.test("Get port info", function()
	local info, err = serial.get_port_info(vars.port_name)
	assert(not err)
	assert(info)
	print(info.type)
end)

taf.test("Reading from serial device", function()
	local err
	local port
	port, err = serial.open(vars.port_name)
	assert(not err)
	assert(port)
	local result
	result, err = serial.read_until(port, "src.-CH8", 4000)
	assert(result)
	print(result)
	assert(err == nil)
end)
