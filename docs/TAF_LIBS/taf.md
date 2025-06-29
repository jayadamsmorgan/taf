# TAF Core Library (`taf`)

The `taf` library is the heart of the Test Automation Framework, providing the essential functions and helpers for defining, running, and logging tests.

## Getting Started

To use the library, you must first require it in your test file.

```lua
local taf = require("taf")
```

---

## API Reference

### Test Definition

Functions for registering and defining your tests.

#### `taf.test(test_name, test_body)`
#### `taf.test(test_name, tags, test_body)`

Registers a new test case. This is the primary function for creating a test.

**Parameters:**
*   `test_name` (`string`): A descriptive name for the test.
*   `tags` (`table` of `string`, optional): A list of tags to categorize the test.
*   `test_body` (`function`): The function containing the test logic.

**Example:**
```lua
taf.test("My First Test", function()
    taf.log_info("This test is starting!")
    -- ... test logic here ...
    taf.log_info("Test finished successfully.")
end)

taf.test("A Tagged Test", {"smoke", "api"}, function()
    taf.log_info("This is a smoke test for the API.")
    -- ...
end)
```

---

### Logging

Functions for outputting information during a test run. All logs are sent to the console TUI and the log files.

#### `taf.log(log_level, ...)`

The main logging function. All other `taf.log_*` functions are shortcuts for this one.

**Parameters:**
*   `log_level` (`string`): The severity level of the log. Can be `"critical"`, `"error"`, `"warning"`, `"info"`, `"debug"`, or `"trace"`. The level can be abbreviated to its first letter (e.g., `"i"`) and is case-insensitive.
*   `...` (`any`): One or more values to be logged, similar to `print()`.

> **Important Behavior:**
> *   `"error"`: Marks the test as **failed**, but allows it to continue executing.
> *   `"critical"`: Immediately **stops and fails** the test.

#### `taf.log_critical(...)`
Logs a message with the `critical` level. **Immediately fails the test.**

#### `taf.log_error(...)`
Logs a message with the `error` level. **Marks the test as failed** but continues execution.

#### `taf.log_warning(...)`
Logs a message with the `warning` level.

#### `taf.log_info(...)`
Logs a message with the `info` level. This is the default level for general information.

#### `taf.print(...)`
An alias for `taf.log_info(...)`. It behaves like the standard Lua `print()` but is integrated with the TAF logging system.

#### `taf.log_debug(...)`
Logs a message with the `debug` level, intended for detailed diagnostic information.

#### `taf.log_trace(...)`
Logs a message with the `trace` level, the most verbose level for tracing execution flow.

**Example:**
```lua
taf.test("Logging demonstration", function()
    taf.print("Starting the logging demo.") -- Same as taf.log_info
    taf.log_debug("This is a debug message with a value:", 123)
    
    -- This will mark the test as failed, but the test will continue
    taf.log_error("An error occurred, but we can proceed.")
    
    taf.print("This line will still be executed.")

    -- This will stop the test immediately
    -- taf.log_critical("A critical failure occurred!") 
end)
```

---

### Test Control & Utilities

Helper functions for managing test flow and timing.

#### `taf.defer(defer_func, ...)`

Registers a function to be executed after the test finishes, regardless of whether it passed or failed.

**Parameters:**
*   `defer_func` (`function`): The function to execute upon completion.
*   `...` (`any`): Arguments to pass to `defer_func` when it is called.

> **Execution Order (LIFO):**
> Defers are executed in a "Last-In, First-Out" order. The last defer registered is the first one to run.
> If a test fails, only the defers registered *before* the failure point will be executed.

> **If no arguments passed after `defer_func`, TAF will pass `status` argument to the defer, which is a string with two values: `"passed"` or `"failed"`**

**Example:**
```lua
taf.test("Defer demonstration", function()
    taf.print("Opening a resource...")
    taf.defer(function(status)
        taf.print("Closing resource with status:", status)
    end)

    taf.print("Creating a temporary file...")
    taf.defer(function()
        taf.print("Deleting temporary file.")
    end)

    taf.print("Opening important port...")
    local port = open_very_important_port()
    taf.defer(close_port, port)                     -- inline version
    taf.defer(taf.print, "Closing important port.") -- inline version
    
    taf.print("Test logic is running.")
end)
-- Test Output:
-- > Opening a resource...
-- > Creating a temporary file...
-- > Opening important port...
-- > Test logic is running.
-- > Closing important port.              <-- Fourth defer
-- > Deleting temporary file.             <-- Second defer
-- > Closing resource with status: passed <-- First defer
```

#### `taf.get_current_target()`

Gets the name of current test target executing if project is multitarget. Otherwise returns an empty string.

**Returns:**
*   (`string`): Target currently executing.

#### `taf.sleep(ms)`

Pauses the test execution for a specified duration.

**Parameters:**
*   `ms` (`number`): The time to sleep, in milliseconds.

#### `taf.millis()`

Gets the number of milliseconds that have elapsed since the current test started.

**Returns:**
*   (`number`): The elapsed time in milliseconds.

**Example:**
```lua
taf.test("Timing test", function()
    taf.sleep(100)
    local elapsed = taf.millis()
    taf.print("Time elapsed after sleep:", elapsed, "ms")
end)
```
