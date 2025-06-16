# JSON Utilities (`taf.json`)

The `taf.json` library provides fast and flexible functions for serializing Lua tables to JSON strings and deserializing JSON strings back into Lua tables.

## Getting Started

The `json` library is exposed as a submodule of the main `taf` object.

```lua
local taf = require("taf")
local json = taf.json -- Access the JSON module
```

## API Reference

### `taf.json.serialize(object, opts)`

Converts a Lua value (typically a table) into a JSON formatted string.

**Parameters:**
*   `object` (`any`): The Lua value to serialize.
*   `opts` (`json_serialize_opts`, optional): A table of options to control the output formatting.

**Returns:**
*   (`string`): The resulting JSON string.

#### `json_serialize_opts` (table)

The options table allows you to customize the appearance of the output JSON.

| Field | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `pretty` | `boolean` | `false` | If `true`, formats the output with newlines and indentation for human readability. |
| `pretty_tab` | `boolean` | `false` | If `true` and `pretty` is also `true`, uses tabs for indentation instead of the default 2 spaces. |
| `spaced` | `boolean` | `false` | If `true`, adds spaces between keys, colons, and values in objects (e.g., `{ "key" : "value" }`). |
| `color` | `boolean`| `false` | If `true`, adds terminal escape codes to colorize the JSON output. Useful for printing to the console. |
| `no_trailing_zero`| `boolean`| `false`| If `true`, removes trailing zeros from floating-point numbers (e.g., `1.200` becomes `1.2`). |
| `slash_escape` | `boolean`| `false`| By default, slashes are **not** escaped. Set to `true` to escape forward slashes (e.g., `/` becomes `\/`). |

**Example:**
```lua
taf.test("JSON Serialization", function()
    local my_data = {
        name = "TAF Test",
        id = 123,
        active = true,
        tags = {"core", "api"},
        path = "c:/temp/data"
    }

    -- Default compact serialization
    local compact_json = json.serialize(my_data)
    taf.print("Compact JSON:", compact_json)
    -- Output: {"name":"TAF Test","id":123,"active":true,"tags":["core","api"],"path":"c:/temp/data"}

    -- Pretty-printed serialization
    local pretty_json = json.serialize(my_data, { pretty = true, slash_escape = true })
    taf.print("Pretty JSON with escaped slashes:")
    taf.print(pretty_json)
    -- Output:
    -- {
    --   "name": "TAF Test",
    --   "id": 123,
    --   "active": true,
    --   "tags": [
    --     "core",
    --     "api"
    --   ],
    --   "path": "c:\/temp\/data"
    -- }
end)
```

### `taf.json.deserialize(str)`

Parses a JSON formatted string and converts it into a Lua table.

**Parameters:**
*   `str` (`string`): The JSON string to parse.

**Returns:**
*   (`table`): The resulting Lua table. Will raise an error if the JSON is invalid.

**Example:**
```lua
taf.test("JSON Deserialization", function()
    local json_string = '{"user": "test_user", "permissions": ["read", "write"], "session_id": 98765}'
    
    local data = json.deserialize(json_string)
    
    taf.print("User:", data.user)
    taf.print("First permission:", data.permissions[1])
    
    -- You can now work with the Lua table
    if data.session_id > 90000 then
        taf.log_info("High session ID detected.")
    end
end)
```
