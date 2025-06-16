# Test Teardown with `taf.defer`

In testing, it's crucial to clean up resources—like closing files, shutting down network connections, or deleting temporary data—at the end of a test. The `taf.defer` function provides a robust and reliable mechanism to ensure this cleanup happens consistently.

`taf.defer` registers a function to be executed immediately after the current test case finishes, regardless of its outcome.

## Guaranteed Cleanup

The primary purpose of `taf.defer` is to guarantee that cleanup code runs, even if the test fails unexpectedly.

```lua
local taf = require("taf")

taf.test("Resource cleanup example", function()
    taf.log_info("Opening a resource...")
    local resource = open_critical_resource()
    assert(resource, "Test cannot continue if resource failed to open")

    -- This defer is now registered.
    taf.defer(function()
        taf.log_info("Closing the critical resource.")
        close_critical_resource(resource)
    end)

    taf.log_info("Performing actions with the resource...")
    -- ... more test logic ...
    
    -- Let's imagine the test fails here
    -- taf.log_critical("Something went wrong!")
end)
```

> **Important:** It does not matter if the test passes or fails; once a defer function is registered, it will be invoked. However, if the test exits *before* the `taf.defer` line is reached (like in our `assert` example), the defer function will not have been registered and will not run.

## Execution Order: Last-In, First-Out (LIFO)

You can register multiple `defer` functions within a single test. They are executed in a "Last-In, First-Out" (LIFO) order, meaning the last defer registered is the first one to run upon completion.

```lua
taf.test("LIFO defer demonstration", function()
    -- First defer registered
    taf.defer(function()
        print("This defer runs second.")
    end)

    -- Second defer registered
    taf.defer(function()
        print("This defer runs first!")
    end)
    
    print("Test logic is executing...")
end)
```

**Test Output:**
```
Test logic is executing...
This defer runs first!
This defer runs second.
```
This LIFO behavior is useful for handling dependent resources, ensuring things are cleaned up in the reverse order of their creation.

## Conditional Cleanup with `status`

A deferred function can optionally accept a single argument, which TAF will provide. This argument is a string containing the test's final status: either `"passed"` or `"failed"`.

This allows you to perform conditional logic during teardown, such as saving extra debug information only when a test fails.

```lua
taf.test("Conditional defer example", function()
    taf.defer(function(status)
        if status == "failed" then
            print("Oh no! The test failed. Saving diagnostic data...")
            -- save_debug_logs()
        elseif status == "passed" then
            print("Hooray! The test passed.")
        end
    end)

    -- Test logic that might pass or fail
    local success = perform_complex_operation()
    if not success then
        taf.log_error("The complex operation failed!")
    end
end)
```

## Simplified Syntax: Passing Arguments Directly

For simple cleanup calls, you can use an alternative syntax that avoids writing an anonymous function. Pass the function name as the first argument to `taf.defer`, and any subsequent arguments will be passed directly to that function when it's executed.

```lua
local taf = require("taf")

taf.test("Simplified defer syntax", function()
    local port = open_some_port()
    assert(port, "Port could not be opened")

    -- Default defer style:
    taf.defer(function()
        print("Closing port...")
    end)

    -- Simplified defer style:
    -- Passes the 'port' variable to the 'close_some_port' function on execution.
    taf.defer(close_some_port, port)

    -- This is also valid and is equivalent to the first defer.
    taf.defer(print, "Closing port...")
end)
```

Both styles are fully supported. The simplified syntax can make your code more concise, while the standard function block offers more flexibility for complex logic. Choose the style that you find more readable and elegant for your use case.
