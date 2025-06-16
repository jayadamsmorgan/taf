# Tag System

The TAF tag system is a powerful feature for categorizing, organizing, and selectively running your tests. By assigning tags to your test cases, you can easily create and execute different test suites on the fly.

This is especially useful for:
*   Creating subsets of tests, like `smoke`, `regression`, or `performance`.
*   Running all tests related to a specific feature, like `api`, `ui`, or `login`.
*   Separating tests that require specific hardware or environments.

## Defining Tags in Your Tests

You can assign tags to any test by passing a table of strings as the second argument to the `taf.test` function.

```lua
local taf = require("taf")

-- This test is tagged as both "api" and "smoke"
taf.test("Verify user login endpoint", { "api", "smoke" }, function()
    -- ... Test logic ...
end)

-- This test is only tagged as "api"
taf.test("Check user profile data", { "api" }, function()
    -- ... Test logic ...
end)

-- This test has no tags
taf.test("Simple utility function check", function()
    -- ... Test logic ...
end)
```

> Tags are passed as a simple array-like table of strings: `{ "tag1", "tag2", ... }`.

## Running Tests by Tag

To run only the tests that match specific tags, use the `--tags` or `-t` command-line flag. You can provide a single tag or a comma-separated list of tags.

| Flag | Description |
| :--- | :--- |
| `--tags <tags>` | The full command-line flag. |
| `-t <tags>` | The short-form alias. |

**Example Usage:**

```bash
# Run all tests with the "api" tag
taf test --tags api

# Run all tests with the "smoke" tag (short form)
taf test -t smoke

# Run tests that have EITHER the "api" OR "smoke" tag
taf test -t api,smoke
```

## Understanding the Filtering Logic

**Tag filtering is inclusive (using OR logic).**

This means that a test will be selected to run if it has **at least one** of the tags you provide on the command line.

Let's use our code example from above to see how this works in practice.

#### Scenario 1: Running with a specific feature tag

```bash
taf test -t api
```
*   **Result:** This will execute both "Verify user login endpoint" and "Check user profile data" because both tests include the `api` tag.

#### Scenario 2: Running with a suite tag

```bash
taf test -t smoke
```
*   **Result:** This will **only** execute "Verify user login endpoint" because it's the only test with the `smoke` tag.

#### Scenario 3: Running with multiple tags

```bash
taf test -t api,smoke
```
*   **Result:** This will execute both "Verify user login endpoint" and "Check user profile data".
    *   The first test runs because it has both `api` and `smoke` tags.
    *   The second test runs because it has the `api` tag.
*   The un-tagged test, "Simple utility function check," will not run in any of these scenarios. It will only run if you execute `taf test` without any tag filters.
