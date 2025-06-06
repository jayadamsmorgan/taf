#ifndef MODULE_TAF_H
#define MODULE_TAF_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#define DEFER_LIST_KEY "taf.defer.list"

/******************* API START ***********************/

// taf:defer(defer_func: function, ...)
// taf:defer(defer_func: function(status: string))
int l_module_taf_defer(lua_State *L);

// taf:log(log_level: string, ...)
int l_module_taf_log(lua_State *L);

// taf:millis() -> ms: number
int l_module_taf_millis(lua_State *L);

// taf:print(...)
int l_module_taf_print(lua_State *L);

// taf:test(name: string, body: function)
// taf:test(name: string, tags: [string], body: function)
int l_module_taf_register_test(lua_State *L);

/******************* API END *************************/

// taf:sleep(ms: number)
int l_module_taf_sleep(lua_State *L);

// Register "taf-main" module
int l_module_taf_register_module(lua_State *L);

#endif // MODULE_TAF_H
