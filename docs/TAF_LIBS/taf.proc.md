# Process Management (`taf.proc`)

The `taf.proc` library provides functions to run and interact with external system processes. It allows you to execute commands synchronously, capturing their output, or spawn them asynchronously for more complex interactions like streaming I/O.

## Getting Started

The `proc` library is exposed as a submodule of the main `taf` object.

```lua
local taf = require("taf")
local proc = taf.proc -- Access the process management module
```

---

## API Reference

### High-Level Execution

#### `taf.proc.run(opts, timeout, sleepinterval)`

Runs an external command, waits for it to complete (with an optional timeout), and captures its standard output, standard error, and exit code. This is the simplest and most common way to run an external process.

**Parameters:**
*   `opts` (`run_opts`): A table specifying the executable and its arguments.
*   `timeout` (`integer`, optional): The maximum time to wait for the process to complete, in milliseconds. If omitted, it will wait indefinitely.
*   `sleepinterval` (`integer`, optional): The interval in milliseconds to check for process completion when a timeout is used. Defaults to `20`.

**Returns:**
*   `result` (`run_result`): A table containing the `stdout`, `stderr`, and `exitcode`.
*   `err` (`string`, optional): If a timeout occurs, this will be the string `"timeout"`. In this case, `result.exitcode` will be `-1`.

**Example:**
```lua
taf.test("Run a git command", function()
    local result, err = proc.run({
        exe = "git",
        args = {"--version"}
    }, 2000) -- 2-second timeout

    if err then
        taf.log_critical("Command timed out:", err)
    end

    if result.exitcode == 0 then
        taf.log_info("Git command successful!")
        taf.print("Output:", result.stdout)
    else
        taf.log_error("Git command failed with code:", result.exitcode)
        taf.print("Error output:", result.stderr)
    end
end)
```

---

### Asynchronous Spawning

For advanced use cases where you need to interact with a process while it is running.

#### `taf.proc.spawn(opts)`

Spawns an external process and immediately returns a handle for interacting with it asynchronously. This allows you to write to its `stdin` and read from its `stdout`/`stderr` while it is still running.

**Parameters:**
*   `opts` (`run_opts`): A table specifying the executable and its arguments.

**Returns:**
*   (`proc_handle`): An object for controlling the spawned process.

**Example:**
```lua
taf.test("Interact with a running process", function()
    -- Spawn 'grep' to search for "Hello"
    local handle = proc.spawn({
        exe = "grep",
        args = {"Hello"}
    })

    -- Defer killing the process to ensure cleanup
    taf.defer(handle.kill, handle)

    -- Write data to the process's stdin
    handle:write("Line 1\n")
    handle:write("Hello World\n")
    handle:write("Line 3\n")
    
    -- Close stdin by writing an empty string (or handle:write()) - this may be platform dependent
    handle:write("") 

    -- Wait for the process to finish
    while handle:wait() == nil do
        taf.sleep(50)
    end

    -- Read the output
    local output = handle:read()
    taf.log_info("Grep found:", output) -- Expected: "Hello World\n"
end)
```

---

### The `proc_handle` Object

The `proc_handle` object is returned by `taf.proc.spawn()` and provides methods for controlling a running process.

#### `handle:read(stream, want)`

Reads from the process's standard output or standard error stream.

**Parameters:**
*   `stream` (`proc_output_stream`, optional): The stream to read from. Defaults to `"stdout"`.
*   `want` (`integer`, optional): The maximum number of bytes to read. Defaults to `4096`.

**Returns:**
*   (`string`): The data read from the stream.

#### `handle:write(buf)`

Writes data to the process's standard input stream.

**Parameters:**
*   `buf` (`string`): The buffer of data to write.

**Returns:**
*   (`integer`): The number of bytes successfully written.

#### `handle:wait()`

Performs a non-blocking check to see if the process has exited.

**Returns:**
*   (`integer`): The process's exit code if it has terminated.
*   (`nil`): If the process is still running.

#### `handle:kill()`

Sends a termination signal (SIGINT) to the process to gracefully shut it down.

---

### Data Structures & Types

#### `run_opts` (table)

Specifies the command to be executed.

| Field | Type | Description |
| :--- | :--- | :--- |
| `exe` | `string` | The path to the executable or its name (if in the system's PATH). |
| `args` | `table` of `string` | (Optional) A list of command-line arguments to pass to the executable. |

#### `run_result` (table)

Contains the results of a completed process, returned by `taf.proc.run()`.

| Field | Type | Description |
| :--- | :--- | :--- |
| `stdout` | `string` | All data captured from the process's standard output. |
| `stderr` | `string` | All data captured from the process's standard error. |
| `exitcode`| `integer`| The integer exit code of the process. |

#### `proc_output_stream` (alias)

Defines the output stream to read from in `handle:read()`.

| Value | Description |
| :--- | :--- |
| `"stdout"` | The standard output stream (default). |
| `"stderr"` | The standard error stream. |
