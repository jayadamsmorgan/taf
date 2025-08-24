#ifndef MODULE_TAF_H
#define MODULE_TAF_H

#include "taf_state.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#define DEFER_LIST_KEY "taf.defer.list"

/******************* API START ***********************/

// taf:defer(defer_func: function, ...)
// taf:defer(defer_func: function(status: string))
int l_module_taf_defer(lua_State *L);

// taf:get_current_target() -> target: string
int l_module_taf_get_current_target(lua_State *L);

// taf:get_active_tags() -> tags: [string]
int l_module_taf_get_active_tags(lua_State *L);

// taf:get_active_test_tags() -> tags: [string]
int l_module_taf_get_active_test_tags(lua_State *L);

// taf:get_var(var_name:string) -> value:string
int l_module_taf_get_var(lua_State *L);

// taf:get_vars() -> [table<string, string>]
int l_module_taf_get_vars(lua_State *L);

// taf:log(log_level: string, ...)
int l_module_taf_log(lua_State *L);

// taf:millis() -> ms:integer
int l_module_taf_millis(lua_State *L);

// taf:print(...)
int l_module_taf_print(lua_State *L);

// reg_opts:
// - name: string
// - tags: [string]?
// - description: string?
// - vars: [string]?
// - body: fun()
//
// taf:test(opts: reg_opts)
int l_module_taf_register_test(lua_State *L);

// taf_var:
// - values: [string]?
// - default: string?
//
// taf:register_vars(vars: table<string, taf_var>)
int l_module_taf_register_vars(lua_State *L);

// taf:register_secrets(secrets: [string])
int l_moduel_taf_register_secrets(lua_State *L);

// taf:sleep(ms: number)
int l_module_taf_sleep(lua_State *L);

/******************* API END *************************/

// Register "taf-main" module
int l_module_taf_register_module(lua_State *L);

void l_module_taf_init(taf_state_t *state);

#endif // MODULE_TAF_H
