# Web Browser Automation (`taf.webdriver`)

The `taf.webdriver` library provides a powerful interface for automating web browsers using the W3C WebDriver standard. It allows you to programmatically control browsers like Chrome, Firefox, and Safari to perform end-to-end testing of web applications.

This module acts as a client for WebDriver servers (e.g., `chromedriver`, `geckodriver`). You must have the appropriate driver for your browser installed and available in your system's PATH.

## Getting Started

The `webdriver` library is exposed as a submodule of the main `taf` object.

```lua
local taf = require("taf")
local webdriver = taf.webdriver -- Access the webdriver module
```

## API Reference

The typical workflow involves three main stages:
1.  **Spawn a driver:** Start the WebDriver server process.
2.  **Manage a session:** Start a browser session, perform actions, and then end the session.
3.  **Kill the driver:** Terminate the WebDriver server process.

### Driver Management

#### `taf.webdriver.spawn_webdriver(webdriver, port, extraflags)`

Spawns a WebDriver server process that listens for commands on a specific port. This does *not* open a browser window yet.

**Parameters:**
*   `webdriver` (`string` or `webdriver` alias): The name or path of the WebDriver executable (e.g., `"chromedriver"`).
*   `port` (`integer`): The network port for the driver to listen on (e.g., `9515` for chromedriver, `4444` for selenium).
*   `extraflags` (`table` of `string`, optional): A list of additional command-line flags to pass to the driver executable.

**Returns:**
*   (`proc_handle`): A process handle from `taf.proc` for the spawned driver. You should use `taf.defer` to ensure `handle:kill()` is called.

### Session Management

#### `taf.webdriver.session_start(opts)`

Connects to a running WebDriver server and starts a new browser session, which opens a browser window.

**Parameters:**
*   `opts` (`wd_session_opts`): A table containing the connection details.

**Returns:**
*   (`session`): A session object that must be passed to all subsequent browser command functions.

#### `taf.webdriver.session_end(session)`

Closes the browser window and ends the specified WebDriver session. It's crucial to call this using `taf.defer` to ensure the browser is always closed.

**Parameters:**
*   `session` (`session`): The active session object to terminate.

---

### The `session` Object

The `session` object is your handle to an active browser instance. It is returned by `session_start` and is the **first argument** to nearly all browser interaction functions.

| Field | Type | Description |
| :--- | :--- | :--- |
| `base_url` | `string` | The base URL of the WebDriver server (e.g., `http://localhost`). |
| `port` | `integer` | The port of the WebDriver server. |
| `id` | `string` | The unique session ID for this browser instance. |

---

### Browser Navigation

#### `taf.webdriver.open_url(session, url)`
Navigates the browser to a new URL.

#### `taf.webdriver.get_current_url(session)`
**Returns:** (`string`) The current URL of the page.

#### `taf.webdriver.get_title(session)`
**Returns:** (`string`) The current title of the page.

#### `taf.webdriver.go_back(session)`
Navigates one step backward in the browser's history.

#### `taf.webdriver.go_forward(session)`
Navigates one step forward in the browser's history.

#### `taf.webdriver.refresh(session)`
Reloads the current page.

### Element Interaction

These functions allow you to find and interact with elements on the page.

#### `taf.webdriver.find_element(session, using, value)`

Finds the first element on the page matching the given selector.

**Parameters:**
*   `session` (`session`): The active session.
*   `using` (`string`): The selector strategy (e.g., `"css selector"`, `"xpath"`, `"link text"`).
*   `value` (`string`): The selector value.

**Returns:**
*   (`string`): An `element_id` for the found element, used in other interaction functions.

#### `taf.webdriver.find_elements(session, using, value)`
Finds all elements on the page matching the given selector.
**Returns:**
*   (`table` of `string`): A list of `element_id`s.

#### `taf.webdriver.click(session, element_id)`
Clicks on a specified element.

#### `taf.webdriver.send_keys(session, element_id, text)`
Types a string of text into an element (e.g., an input field).

#### `taf.webdriver.get_text(session, element_id)`
**Returns:** (`string`) The visible text content of an element.

### Advanced Usage

#### `taf.webdriver.execute(session, script, args)`
Executes synchronous JavaScript within the context of the current page.

#### `taf.webdriver.screenshot(session)`
Takes a screenshot of the current page.
**Returns:** (`string`) A Base64-encoded string of the PNG image.

#### `taf.webdriver.session_cmd(session, method, endpoint, payload)`
A low-level helper for sending a raw command to a WebDriver session endpoint. All other functions are built on top of this.

---

### Data Structures & Types

#### `wd_session_opts` (table)
Options for `taf.webdriver.session_start`.

| Field | Type | Description |
| :--- | :--- | :--- |
| `port` | `integer` | **Required.** The port of the running WebDriver server. |
| `url` | `string` | (Optional) The URL of the WebDriver server. Defaults to `http://localhost`. |

---

### Full Example

This example demonstrates the complete lifecycle of a WebDriver test: spawning the driver, starting a session, interacting with a page, and cleaning up.

```lua
local taf = require("taf")
local webdriver = taf.webdriver

taf.test("Google Search with WebDriver", {"e2e", "smoke"}, function()
    -- 1. Spawn the driver process
    taf.log_info("Spawning chromedriver on port 9515...")
    local driver_handle = webdriver.spawn_webdriver("chromedriver", 9515)
    -- Defer killing the driver to ensure it's always shut down
    taf.defer(driver_handle.kill, driver_handle)
    
    -- Give the driver a moment to start up
    taf.sleep(2000) 

    -- 2. Start a browser session
    taf.log_info("Starting new browser session...")
    local session = webdriver.session_start({ port = 9515 })
    -- Defer ending the session to ensure the browser is always closed
    taf.defer(webdriver.session_end, session)

    -- 3. Perform browser actions
    webdriver.open_url(session, "https://www.google.com")
    taf.log_info("Opened google.com. Title: " .. webdriver.get_title(session))

    -- Find the search box element by its CSS selector
    local search_box_id = webdriver.find_element(session, "css selector", "textarea[name='q']")
    taf.log_info("Found search box element with ID:", search_box_id)
    
    -- Type into the search box
    webdriver.send_keys(session, search_box_id, "Test Automation Framework")
    
    -- Find and click the search button
    local search_btn_id = webdriver.find_element(session, "css selector", "input[name='btnK']")
    webdriver.click(session, search_btn_id)
    taf.log_info("Clicked search button.")
    
    -- Wait for search results to load
    taf.sleep(3000)
    
    -- 4. Verify the result
    local new_title = webdriver.get_title(session)
    taf.log_info("Results page title:", new_title)
    if not new_title:find("Test Automation Framework") then
        taf.log_error("Page title did not update as expected.")
    end

    -- 5. Cleanup happens automatically via deferred calls
    taf.log_info("Test finished. Cleanup will now run.")
end)
```
