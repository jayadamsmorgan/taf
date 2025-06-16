# TAF Command-Line Interface (CLI)

The `taf` executable is the primary tool for creating, managing, and running your test projects. This guide details all available commands and their options.

## Main Commands

The `taf` executable uses a command/sub-command structure.

**Usage:**
```bash
taf <command> [sub-command] [arguments...] [options...]
```

| Command | Description |
| :--- | :--- |
| [`init`](#taf-init) | Initialize a new TAF project. |
| [`test`](#taf-test) | Run tests for a project. |
| [`target`](#taf-target) | Manage targets for a multi-target project. |
| [`logs`](#taf-logs) | Parse and display information from TAF log files. |
| `version` | Display the installed TAF version. |
| `help` | Display the main help message. |

---

### `taf init`

Creates a new TAF project directory with the necessary files and structure.

**Usage:**
```bash
taf init <project-name> [options...]
```

#### Arguments
*   `project-name` (required): The name of the new project. A directory with this name will be created.

#### Options
| Option | Alias | Description |
| :--- | :--- | :--- |
| `--multitarget` | `-m` | Initializes the project with a multi-target structure. |
| `--internal-log`| `-i` | Dumps an internal TAF log file for advanced debugging. |
| `--help` | `-h` | Displays the help message for the `init` command. |

#### Examples
```bash
# Create a standard single-target project
taf init my_api_tests

# Create a project designed for multiple hardware targets
taf init my_embedded_project --multitarget
```

---

### `taf test`

Executes the tests within the current project.

**Usage:**
```bash
# For Single-Target projects
taf test [options...]

# For Multi-Target projects
taf test <target_name> [options...]
```

#### Arguments
*   `target_name` (optional): The name of the target to run tests against. This is **required** for multi-target projects.

#### Options
| Option | Alias | Description |
| :--- | :--- | :--- |
| `--log-level <level>` | `-l` | Sets the minimum log level to display in the TUI. Valid levels are `critical`, `error`, `warning`, `info`, `debug`, `trace`. See the [Logging](./Logging.md) documentation for details. |
| `--tags <tags>` | `-t` | Runs only the tests that have at least one of the specified comma-separated tags. See the [Tag System](./Tag-system.md) documentation for details. |
| `--no-logs` | `-n` | Disables the creation of log files for this test run. |
| `--internal-log`| `-i` | Dumps an internal TAF log file for advanced debugging. |
| `--help` | `-h` | Displays the help message for the `test` command. |

#### Examples
```bash
# Run all tests in a single-target project
taf test

# Run only smoke tests
taf test --tags smoke

# Run tests for a specific target in a multi-target project with a verbose log level
taf test my_board_v2 -l debug
```

---

### `taf target`

Manages the targets in a multi-target project. This command requires a sub-command.

#### `taf target add`

Adds a new target to the project and creates its corresponding test directory.

**Usage:**
```bash
taf target add <target_name>
```
*   **Arguments:** `target_name` (required) - The name of the new target to add.

#### `taf target remove`

Removes a target from the project's configuration.

**Usage:**
```bash
taf target remove <target_name>
```
*   **Arguments:** `target_name` (required) - The name of the target to remove.

> **Note:** The `remove` command does not delete the target's test directory, allowing you to manage the files manually.

---

### `taf logs`

Provides utilities for interacting with TAF log files. This command requires a sub-command.

#### `taf logs info`

Parses a raw JSON log file and displays a summary of the test run.

**Usage:**
```bash
taf logs info <path_to_log | latest>
```

#### Arguments
*   `path_to_log | latest` (required): Either the literal string `latest` to parse the most recent log, or the file path to a specific `test_run_[...]_raw.json` file.

#### Example
```bash
# Get a summary of the last test run
taf logs info latest
```
