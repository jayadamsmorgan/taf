#ifndef MODULE_HOOKS_H
#define MODULE_HOOKS_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/******************* API START ***********************/

// hooks:register_test_run_started(fn:fun(context))
int l_module_hooks_register_test_run_started(lua_State *L);

// hooks:register_test_started(fn:fun(context))
int l_module_hooks_register_test_started(lua_State *L);

// hooks:register_test_finished(fn:fun(context))
int l_module_hooks_register_test_finished(lua_State *L);

// hooks:register_test_run_finished(fn:fun(context))
int l_module_hooks_register_test_run_finished(lua_State *L);

/******************* API END *************************/

// Register "taf-hooks" module
int l_module_hooks_register_module(lua_State *L);

#endif // MODULE_HOOKS_H
