local serial = require("serial")

test_case("List serial devices", function()
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

test_case("Get port info", function()
	local info, err = serial.get_port_info("/dev/cu.usbmodem21202")
	assert(not err)
	assert(info)
	print(info.type)
end)

test_case("Reading from serial device", function()
	local err
	local port
	port, err = serial.open("/dev/cu.usbmodem21202")
	assert(not err)
	assert(port)
	local result
	result, err = serial.read_until(port, "src.-CH8", 4000)
	assert(result)
	print(result)
	assert(err == nil)
end)
