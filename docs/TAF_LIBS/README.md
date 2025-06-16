# TAF Core Libraries

Welcome to the documentation for the TAF core libraries. This set of libraries provides the fundamental building blocks for creating robust and powerful automated tests.

The core libraries are accessed through the main `taf` object, which is loaded at the beginning of every test script:

```lua
local taf = require("taf")
```

## Available Modules

Each module provides a distinct set of functionalities. Click on a module name to view its detailed API documentation.

*   [**`taf` (Core Library)**](./taf.md)
    *   **Purpose:** The heart of the framework. Use this for defining tests, logging, controlling test flow with `defer`, and basic utilities like `sleep`.
    *   **Key Functions:** `taf.test()`, `taf.log_info()`, `taf.log_error()`, `taf.defer()`

*   [**`taf.webdriver`**](./taf.webdriver.md)
    *   **Purpose:** End-to-end web browser automation. It provides a client for the W3C WebDriver standard, allowing you to control browsers like Chrome, Firefox, and Safari.
    *   **Key Functions:** `webdriver.spawn_webdriver()`, `webdriver.session_start()`, `webdriver.open_url()`, `webdriver.find_element()`, `webdriver.click()`

*   [**`taf.http`**](./taf.http.md)
    *   **Purpose:** A powerful, low-level HTTP client for making API requests. Based on libcurl, it gives you fine-grained control over every aspect of a network transfer.
    *   **Key Functions:** `http.new()`, `handle:setopt()`, `handle:perform()`

*   [**`taf.proc`**](./taf.proc.md)
    *   **Purpose:** Run and interact with external system processes and command-line tools. Supports both synchronous execution (run and wait) and asynchronous spawning for complex interactions.
    *   **Key Functions:** `proc.run()`, `proc.spawn()`, `handle:read()`, `handle:kill()`

*   [**`taf.serial`**](./taf.serial.md)
    *   **Purpose:** Communicate with hardware devices over serial ports (COM ports, `/dev/tty*`). Useful for testing embedded systems and other hardware.
    *   **Key Functions:** `serial.list_devices()`, `serial.get_port()`, `port:open()`, `port:read_until()`, `port:write_blocking()`

*   [**`taf.json`**](./taf.json.md)
    *   **Purpose:** Fast and simple utilities for serializing Lua tables into JSON strings and deserializing JSON back into Lua tables.
    *   **Key Functions:** `json.serialize()`, `json.deserialize()`
