#ifndef MODULE_TAF_H
#define MODULE_TAF_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#define DEFER_LIST_KEY "taf.defer.list"

int l_module_taf_register_module(lua_State *L);

int l_module_taf_millis(lua_State *L);
int l_module_taf_print(lua_State *L);

#endif // MODULE_TAF_H
