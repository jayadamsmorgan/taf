#ifndef UTIL_LUA_H
#define UTIL_LUA_H

#include <json.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

int lua_table_is_array(lua_State *L, int index, lua_Integer *max_out);

json_object *lua_to_json(lua_State *L, int index);

void json_to_lua(lua_State *L, struct json_object *obj);

static inline int selfshift(lua_State *L) { /* 1 = dot‑call, 2 = colon‑call */
    return lua_istable(L, 1) ? 2 : 1;
}

#endif // UTIL_LUA_H
