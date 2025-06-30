# TAF Project Setup

The Test Automation Framework (TAF) is designed to be flexible, offering two primary project structures to suit your testing needs: **Single-Target** and **Multi-Target**.

## Single-Target Project (Default)

This is the standard project structure, perfect for testing a single application, service, or piece of hardware.

### 📁 Structure

The `taf init` command creates a clean and logical directory layout:

```
your_project/
├── lib/
│   ├── your_custom_library.lua
│   └── variables.lua
├── tests/
│   ├── example_test_1.lua
│   └── example_test_2.lua
└── .taf.json
```

*   **`tests/`**: This is the heart of your project, where all your test files (`*_test.lua`) reside.
*   **`lib/`**: This directory is designed for reusable code, helper functions, and abstractions that can be shared across multiple tests. Keeping this logic separate helps maintain clean and readable test files.
*   **`.taf.json`**: An auto-generated file used internally by TAF to manage your project's configuration.
    > **Warning:** Do not edit `.taf.json` manually. Use `taf` commands to manage project settings.

### 🚀 Initialization

To create a new single-target project, run the `init` command:

```bash
taf init your_project_name
```

This will create the `your_project_name` directory and populate it with the `lib/`, `tests/`, and `.taf.json` files.

> **Note:** Single-Target project may be converted to Multi-Target by [Adding a Target](#adding-a-target)

---

## Multi-Target Project

If your testing involves multiple distinct environments or devices (e.g., several different embedded Linux boards, various API endpoints), the multi-target structure provides the necessary organization.

### 📁 Structure

A multi-target project extends the standard structure by organizing tests into target-specific subdirectories.

```
your_project/
├── lib/
│   └── ...
├── tests/
│   ├── common/
│   │   ├── common_test_1.lua
│   │   └── common_test_2.lua
│   ├── example_target_1/
│   │   └── target_1_tests.lua
│   └── example_target_2/
│       └── target_2_tests.lua
└── .taf.json
```

*   **`tests/common/`**: This special directory holds tests that are generic and can be run against *any* of your defined targets. TAF will always look for tests here.
*   **`tests/<target_name>/`**: Each target you add to the project gets its own directory for tests that are specific to that target.

### 🚀 Initialization

Create a multi-target project using the `--multitarget` flag:

```bash
taf init your_project_name --multitarget
```

This will create the base structure, including the essential `tests/common/` directory.

> **Note:** Multi-Target project may be converted to Single-Target by [Removing a Target](#adding-a-target) when you only have 1 target left

### Managing Targets

You can easily add and remove targets from your project.

#### Adding a Target

Use the `taf target add` command to register a new target and create its test directory:

```bash
taf target add example_target_1
```

#### Removing a Target

Use the `taf target remove` command to unregister a target:

```bash
taf target remove example_target_1
```

> **Note:** This command only removes the target from TAF's internal tracking. It **will not** delete the `tests/example_target_1/` directory or its contents, giving you full control over your files.

---

## A Note on File Naming

TAF does not enforce any specific naming conventions for your test or library files. You are free to name them as you see fit. TAF automatically scans the `tests/` directory (and its subdirectories) to discover all `taf.test()` definitions.

---

# Getting Started: Your First Test

This quickstart guide will walk you through creating and running a simple test.

### 1. Initialize a New Project

First, create a new TAF project using the `init` command and navigate into the newly created directory.

```bash
taf init my_first_taf_project
cd my_first_taf_project
```

### 2. Write Your Test

Next, create a new file inside the `tests/` directory (e.g., `tests/hello_world_test.lua`) and add the following code:

```lua
-- Require the TAF core library
local taf = require("taf")

-- Define a new test case
taf.test("My Very First Test", function()

    taf.log_info("The test is starting...")
    taf.sleep(1000) -- Pause execution for 1000 milliseconds (1 second)

    -- taf.print is an alias for taf.log_info
    taf.print("Hello from TAF!")

    taf.sleep(1000)
    taf.log_info("The test is finishing.")
end)
```

> **`taf.test(name, body)`**
> This is the primary function for registering a new test case.
> *   The first argument is a `string` describing the test's purpose.
> *   The second argument is a `function` containing the actual test logic.

### 3. Run the Test

Save the file and run your test from the project's root directory using the `taf test` command:

```bash
taf test
```

You will see the TAF Terminal UI launch, execute your test, and print the log messages in real-time. The test will wait for 1 second, print "Hello from TAF!", wait another second, and then complete.

Congratulations! You've just written and successfully run your first TAF test.
