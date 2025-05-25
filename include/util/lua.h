#ifndef UTIL_LUA_H
#define UTIL_LUA_H

#include <json.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

json_object *lua_to_json(lua_State *L, int index);

void json_to_lua(lua_State *L, struct json_object *obj);

#endif // UTIL_LUA_H
