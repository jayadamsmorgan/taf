<div align="center">

# TAF
**A Modern Testing & Automation Framework in Lua**

</div>

TAF is a framework for writing functional tests and automations (AT, ATDD, RPA, etc.) using the simple and powerful Lua programming language. It is designed for both developer and QA workflows, providing the tools needed for robust testing of APIs, web frontends, and external hardware.

Leveraging the simplicity of Lua for test scripting and the raw performance of a C-based core engine, **TAF is a reliable, modern, and _blazingly fast_ testing framework.**

## Key Features

*   **💻 Interactive Terminal UI:** Get a real-time, organized view of your test execution, including progress, timings, and live log output.
*   **✍️ Simple & Expressive Syntax:** Define tests with a clear and minimal API (`taf.test(...)`) that gets out of your way.
*   **📚 Batteries-Included Libraries:** A rich set of built-in modules for common automation tasks, including:
    *   **WebDriver:** Browser automation for end-to-end testing.
    *   **HTTP Client:** A powerful, low-level client for API testing.
    *   **Process Management:** Run and interact with external command-line tools.
    *   **Serial Communication:** Test hardware and embedded devices.
*   **🏷️ Flexible Tagging System:** Categorize your tests with tags and selectively run suites from the command line.
*   **🧹 Guaranteed Teardown:** Use the elegant `taf.defer` function to ensure resources are always cleaned up, whether a test passes or fails.
*   **🗂️ Detailed Logging:** Automatically generates human-readable text logs and machine-readable JSON logs for every test run, perfect for CI/CD integration and reporting.

## Getting Started

Three simple steps to run your first test:

1.  **Build the Project**
    Clone the repository and follow the [build instructions](./BUILD.md) to compile the `taf` executable.

2.  **Initialize a New Project**
    Use the `init` command to scaffold a new project directory.
    ```bash
    taf init my_new_project
    cd my_new_project
    ```

3.  **Run the Example Tests**
    Execute the tests using the `test` command.
    ```bash
    taf test
    ```

## Dive Deeper: Full Documentation

This project is fully documented to help you get the most out of TAF. All documentation can be found in the [`docs/`](./docs) directory.

### Quick Links

*   **Getting Started:** [Project Setup & Your First Test](./docs/PROJECT_SETUP.md)
*   **CLI Reference:** [Command-Line Usage](./docs/CLI.md)
*   **Core Libraries:** [API Reference](./docs/TAF_LIBS/README.md)
*   **Key Concepts:**
    *   [Logging System](./docs/LOGGING.md)
    *   [The Tag System](./docs/TAG_SYSTEM.md)
    *   [Test Teardown with `defer`](./docs/TEST_TEARDOWN.md)

## Installation

Currently, TAF must be built from source. Please see the detailed **[Build Instructions](./BUILD.md)**.

## Contributing

Contributions are welcome! Whether it's bug reports, feature requests, or code contributions, please feel free to open an issue or submit a pull request. We would love to hear from you
