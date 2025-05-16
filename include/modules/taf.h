#ifndef MODULE_TAF_H
#define MODULE_TAF_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

int l_module_taf_millis(lua_State *L);

int l_module_taf_register_module(lua_State *L);

#endif // MODULE_TAF_H
