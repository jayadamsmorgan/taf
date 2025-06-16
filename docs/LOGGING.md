# TAF Logging

TAF provides a comprehensive logging system that automatically records the details of every test run. These logs are essential for debugging failures, reviewing test output, and archiving test results.

## Log File Generation

TAF generates log files automatically in the `your_project/logs/` directory after every test run.

*   To disable log file creation for a specific run, use the `--no-logs` or `-n` command-line option:
    ```bash
    taf test --no-logs
    ```
*   For **multi-target projects**, logs are organized into subdirectories for each target: `your_project/logs/your_target/`.

TAF produces two types of log files for each run.

### 📝 Output Log

This file contains the human-readable console output from the test run, exactly as it appears in the TAF Terminal UI (TUI). It's perfect for a quick review of what happened during the test.

*   **Filename:** `test_run_[DATE]-[TIME]_output.log`
*   **Latest Symlink:** A convenience symlink named `test_run_latest_output.log` is always created, pointing to the log file from the most recent test run.

### Raw Log (JSON)

This file is a machine-readable JSON document containing a complete, structured record of the entire test run. It includes every log message regardless of level, test timings, statuses, and other metadata. It is ideal for programmatic analysis, reporting, or integration with other tools.

*   **Filename:** `test_run_[DATE]-[TIME]_raw.json`
*   **Latest Symlink:** A symlink named `test_run_latest_raw.json` always points to the latest raw log.
*   **Schema:** <!-- TODO --> [The schema for the raw log format can be found here.]()

---

## 📶 Log Levels

TAF supports six hierarchical log levels, allowing you to control the verbosity of the output in the TUI and the [Output Log](#-output-log).

The levels, from most to least severe, are:
*   `critical`
*   `error`
*   `warning`
*   `info` (Default)
*   `debug`
*   `trace`

You can set the minimum log level for a run using the `--log-level` (or `-l`) option. Only messages of the specified level or higher will be displayed.

```bash
# Show only warnings, errors, and critical messages
taf test --log-level warning

# Or using the short-form
taf test -l w
```

> **Important:** Changing the log level only affects the TUI and the `output.log` file. The `raw.json` log **always contains all messages from all levels**, making it a complete record for debugging.

By default, TAF runs with the `info` log level. This means `debug` and `trace` messages are hidden from the console and output log unless a more verbose level is explicitly set.

---

## The `taf logs` Utility

TAF includes a command-line utility for quickly parsing and viewing information from log files.

### `taf logs info`

This command parses a [Raw Log](#-raw-log-json) file and presents a concise summary of the test run, including test counts, pass/fail rates, and duration.

**Usage:**

```bash
taf logs info <path_to_raw_log.json | latest>
```

**Examples:**

```bash
# Show info from the most recent test run
taf logs info latest

# Show info from a specific log file
taf logs info logs/test_run_2023-10-27-143000_raw.json
```
