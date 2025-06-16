# Serial Communication (`taf.serial`)

The `taf.serial` library provides a comprehensive interface for communicating with serial port devices (like COM ports or `/dev/tty*` devices). It allows you to list available ports, configure communication parameters, and perform read/write operations within your tests.

## Getting Started

The `serial` library is exposed as a submodule of the main `taf` object.

```lua
local taf = require("taf")
local serial = taf.serial -- Access the serial module
```

## API Reference

### Device Discovery

Functions to find and select serial ports available on the system.

#### `taf.serial.list_devices()`

Lists all serial ports detected on the system.

**Returns:**
*   (`table`): An array of `serial_port_info` objects, one for each detected device.

**Example:**
```lua
local taf = require("taf")
local serial = taf.serial

taf.test("List all serial ports", function()
    local devices = serial.list_devices()
    if #devices > 0 then
        taf.print("Found", #devices, "serial devices:")
        for _, device in ipairs(devices) do
            taf.print(("- %s (%s)"):format(device.path, device.description))
        end
    else
        taf.log_warning("No serial devices found.")
    end
end)
```

#### `taf.serial.get_port(path)`

Gets a handle for a specific serial port by its path (e.g., `"COM3"` or `"/dev/ttyUSB0"`). This function retrieves the handle but does not open the port for communication.

**Parameters:**
*   `path` (`string`): The system path or name of the serial port.

**Returns:**
*   (`serial_port`): A `serial_port` object handle used for all further interactions.

---

### The `serial_port` Object

The `serial_port` object is the main handle for interacting with an individual serial device. It is obtained by calling `taf.serial.get_port(path)` and contains methods for controlling and communicating with the port.

#### `port:open(mode)`

Opens the serial port for communication.

**Parameters:**
*   `mode` (`serial_mode`): The mode to open the port in (`"r"`, `"w"`, or `"rw"`).

#### `port:close()`

Closes the serial port. It's highly recommended to call this using `taf.defer` to ensure the port is closed even if the test fails.

#### `port:read_blocking(byte_amount, timeout)`

Reads a specified number of bytes from the port, blocking execution until the data is received or a timeout occurs.

**Parameters:**
*   `byte_amount` (`integer`): The number of bytes to read.
*   `timeout` (`integer`, optional): The maximum time to wait in milliseconds. If `0` or `nil`, it will wait indefinitely.

**Returns:**
*   (`string`): The data read from the port.

#### `port:read_nonblocking(byte_amount)`

Attempts to read a specified number of bytes from the port without blocking. Returns immediately with any available data.

**Parameters:**
*   `byte_amount` (`integer`): The maximum number of bytes to read.

**Returns:**
*   (`string`): The data read from the port, which may be empty or less than `byte_amount`.

#### `port:read_until(pattern, timeout, chunk_size)`

Reads from the serial port in chunks until a specific string pattern is found. This is a powerful helper for reading line-based or delimited data.

**Parameters:**
*   `pattern` (`string`, optional): The pattern to search for. Defaults to a newline character (`"\n"`).
*   `timeout` (`number`, optional): Total time to wait in milliseconds.
    *   `nil` (default): Block indefinitely until the pattern is found.
    *   `0`: Perform a single non-blocking read and check.
    *   `> 0`: Wait up to this duration for the pattern to appear.
*   `chunk_size` (`number`, optional): The size of each read chunk in bytes. Defaults to `64`.

**Returns:**
*   (`string`): The complete string read from the port, including the pattern. Throws a timeout error if the pattern is not found within the specified time.

#### `port:write_blocking(data, timeout)`

Writes a string of data to the port, blocking until the write is complete or a timeout occurs.

**Parameters:**
*   `data` (`string`): The data to write.
*   `timeout` (`integer`): The maximum time to wait in milliseconds.

**Returns:**
*   (`integer`): The number of bytes written.

#### `port:write_nonblocking(data)`

Writes a string of data to the port without blocking.

**Parameters:**
*   `data` (`string`): The data to write.

**Returns:**
*   (`integer`): The number of bytes written.

#### Port Configuration Methods

These methods configure the communication parameters of the open serial port.

*   `port:set_baudrate(baudrate)`: Sets the baud rate (e.g., `9600`, `115200`).
*   `port:set_bits(bits)`: Sets the number of data bits (`serial_data_bits`).
*   `port:set_parity(parity)`: Sets the parity mode (`serial_parity`).
*   `port:set_stopbits(stopbits)`: Sets the number of stop bits (`serial_stop_bits`).
*   `port:set_flowcontrol(flowctrl)`: Sets the hardware/software flow control method (`serial_flowctrl`).
*   `port:set_rts(rts_option)`: Sets the Request to Send (RTS) line behavior (`serial_rts`).
*   `port:set_dtr(dtr_option)`: Sets the Data Terminal Ready (DTR) line behavior (`serial_dtr`).
*   `port:set_cts(cts_option)`: Sets the Clear to Send (CTS) line behavior (`serial_cts`).
*   `port:set_dsr(dsr_option)`: Sets the Data Set Ready (DSR) line behavior (`serial_dsr`).
*   `port:set_xon_xoff(xonxoff_option)`: Configures XON/XOFF software flow control (`serial_xonxoff`).

#### Port Status & Control

*   `port:flush(direction)`: Discards data from the input, output, or both buffers (`serial_flush_direction`).
*   `port:drain()`: Waits until all data in the output buffer has been transmitted.
*   `port:get_waiting_input()`: Returns the number of bytes available in the input buffer.
*   `port:get_waiting_output()`: Returns the number of bytes waiting in the output buffer.
*   `port:get_port_info()`: Returns a `serial_port_info` table with detailed information about the specific port instance.

#### Full Example
```lua
local taf = require("taf")
local serial = taf.serial

taf.test("Communicate with GPS Device", {"hardware", "gps"}, function()
    -- Find a specific device by its USB product string
    local devices = serial.list_devices()
    local gps_path
    for _, dev in ipairs(devices) do
        if dev.product and dev.product:find("GPS") then
            gps_path = dev.path
            break
        end
    end

    if not gps_path then
        taf.log_critical("GPS device not found!")
    end

    taf.log_info("Found GPS device at:", gps_path)
    local port = serial.get_port(gps_path)

    -- Ensure the port is closed at the end of the test
    taf.defer(function()
        port:close()
        taf.print("GPS port closed.")
    end)

    -- Open and configure the port
    port:open("rw")
    port:set_baudrate(9600)
    port:set_bits(8)
    port:set_parity("none")
    port:set_stopbits(1)
    
    taf.print("Port configured. Waiting for NMEA sentence...")

    -- Read until we get a GPGGA sentence, with a 5-second timeout
    local sentence = port:read_until("$GPGGA", 5000)

    if sentence:find("$GPGGA") then
        taf.log_info("Received GPGGA sentence:", sentence)
    else
        taf.log_error("Did not receive a GPGGA sentence in time.")
    end
end)
```

---

### Data Structures & Types

#### `serial_port_info` (table)

A table containing detailed information about a serial port, returned by `taf.serial.list_devices()` and `port:get_port_info()`.

| Field | Type | Description |
| :--- | :--- | :--- |
| `path` | `string` | Path or name of the serial port (e.g., `COM3`, `/dev/ttyS0`). |
| `type` | `serial_port_type` | The type of port (`"native"`, `"usb"`, `"bluetooth"`, `"unknown"`). |
| `description` | `string` | A human-readable description of the device. |
| `serial` | `string` (optional) | Serial number, if it's a USB device. |
| `product` | `string` (optional) | Product name string, if it's a USB device. |
| `manufacturer` | `string` (optional) | Manufacturer name string, if it's a USB device. |
| `vid` | `number` (optional) | Vendor ID, if it's a USB device. |
| `pid` | `number` (optional) | Product ID, if it's a USB device. |
| `usb_address` | `number` (optional) | USB port address, if it's a USB device. |
| `usb_bus` | `number` (optional) | USB bus number, if it's a USB device. |
| `bluetooth_address`| `string` (optional) | MAC address, if it's a Bluetooth device. |


#### Type Aliases

These aliases define the set of accepted values for various function parameters.

| Alias Name | Accepted Values | Description |
| :--- | :--- | :--- |
| `serial_mode` | `"r"`, `"w"`, `"rw"` | Read, Write, or Read/Write mode for opening a port. |
| `serial_flush_direction`| `"i"`, `"o"`, `"io"` | Flush the Input, Output, or both buffers. |
| `serial_data_bits` | `5`, `6`, `7`, `8` | Number of data bits. |
| `serial_parity` | `"none"`, `"odd"`, `"even"`, `"mark"`, `"space"` | Parity checking mode. |
| `serial_stop_bits` | `1`, `2` | Number of stop bits. |
| `serial_flowctrl` | `"dtrdsr"`, `"rtscts"`, `"xonxoff"`, `"none"` | Flow control method. |
| `serial_rts` | `"off"`, `"on"`, `"flowctrl"` | Behavior of the RTS line. |
| `serial_dtr` | `"off"`, `"on"`, `"flowctrl"` | Behavior of the DTR line. |
| `serial_cts` | `"ignore"`, `"flowctrl"` | Behavior of the CTS line. |
| `serial_dsr` | `"ignore"`, `"flowctrl"` | Behavior of the DSR line. |
| `serial_xonxoff` | `"i"`, `"o"`, `"io"`, `"disable"` | Software flow control on input, output, both, or disabled. |
| `serial_port_type` | `"native"`, `"usb"`, `"bluetooth"`, `"unknown"`| The physical type of serial port connection. |
