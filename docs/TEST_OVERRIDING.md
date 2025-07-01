# Test Overriding Behavior

The Test Automation Framework (TAF) uses a test's name as its unique identifier. If TAF discovers multiple tests with the exact same name during its discovery phase, it will **override** the earlier definitions and only execute the version of the test that was registered last.

This behavior, while applicable in all project types, is most powerful and intentionally useful in **Multi-Target Projects** for creating target-specific implementations of common tests.

## Test Discovery and Execution Order

To understand overriding, it's essential to know the order in which TAF discovers and loads files. TAF scans and executes Lua files recursively, iterating through directories and files alphabetically at each level.

#### Single-Target Project Order

1.  **`lib/` directory:** All `.lua` files are executed alphabetically.
2.  **`tests/` directory:** All `.lua` files are executed alphabetically.

#### Multi-Target Project Order (when running for `<target_name>`)

1.  **`lib/` directory:** All `.lua` files are executed alphabetically.
2.  **`tests/common/` directory:** All `.lua` files are executed alphabetically.
3.  **`tests/<target_name>/` directory:** All `.lua` files are executed alphabetically.

The test runner collects all `taf.test()` definitions as it executes these files. If a test name is re-registered, the new test body and tags replace the old ones.

## Use Case: Overriding Common Tests in a Multi-Target Project

The primary use case for this behavior is to provide a default test implementation in the `tests/common/` directory that can be selectively overridden by a specific target.

Imagine you have a common test to check the system's kernel version, but one of your targets is a unique device that requires a special method to retrieve this information.

#### 1. Define the Common Test

First, you create the generic test in the `tests/common/` directory.

**File: `tests/common/system_checks.lua`**
```lua
local taf = require("taf")

taf.test("Verify Kernel Version", { "system", "smoke" }, function()
    taf.log_info("Running the standard kernel version check...")
    
    -- Standard command to get kernel version
    local result = taf.proc.run({ exe = "uname", args = {"-r"} })

    if result.exitcode ~= 0 then
        taf.log_critical("Failed to run 'uname -r'")
    end

    local version = result.stdout
    taf.log_info("Detected kernel version:", version)
    
    -- Assert that the version starts with "5."
    if not version:match("^5.") then
        taf.log_error("Expected a 5.x kernel, but got " .. version)
    end
end)
```

This test will run for every target by default.

#### 2. Override the Test for a Specific Target

Now, for your special target named `special_device`, you need a different implementation. You create a new test file in that target's directory and use the **exact same test name**.

**File: `tests/special_device/custom_checks.lua`**
```lua
local taf = require("taf")

-- This test will OVERRIDE the one defined in tests/common/
taf.test("Verify Kernel Version", { "system", "special" }, function()
    taf.log_info("Running the CUSTOM kernel version check for special_device...")
    
    -- A custom command or method for this specific device
    local result = taf.proc.run({ exe = "get_special_kernel_version.sh" })

    if result.exitcode ~= 0 then
        taf.log_critical("Failed to run custom kernel script")
    end

    local version = result.stdout
    taf.log_info("Detected special kernel version:", version)
    
    -- This device uses a 4.x kernel
    if not version:match("^4.") then
        taf.log_error("Expected a 4.x kernel for this device, but got " .. version)
    end
end)
```

### Execution Outcome

*   **When you run `taf test normal_device`:**
    *   TAF loads `tests/common/system_checks.lua`.
    *   The test "Verify Kernel Version" is registered.
    *   TAF executes the **common** version of the test.

*   **When you run `taf test special_device`:**
    *   TAF first loads `tests/common/system_checks.lua`, and the common test is registered.
    *   Then, TAF loads `tests/special_device/custom_checks.lua`. The test "Verify Kernel Version" is discovered again.
    *   The framework **overwrites** the previous definition with this new one (including its new tags: `{ "system", "special" }`).
    *   TAF executes the **custom** version of the test defined in the target's directory.

This powerful mechanism allows you to maintain a clean, shared test suite in `common` while handling exceptions and target-specific logic in a clear and organized way.
