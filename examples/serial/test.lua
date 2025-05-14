local serial = require("serial")

test_case("Connect to serial", function()
	local port, err = serial.open("/dev/tty.usbmodem21202", {
		mode = "r",
		baudrate = 115200,
	})
	if err then
		print(err)
	end
end)
