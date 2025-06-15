#include "modules/json/taf-json.h"

#include "internal_logging.h"

#include "util/lua.h"

int l_module_json_serialize(lua_State *L) {
    int s = selfshift(L);
    luaL_checktype(L, s, LUA_TTABLE);
    int flags = luaL_checkinteger(L, s + 1);

    json_object *obj = lua_to_json(L, s);
    if (!obj) {
        LOG("Unable to create json object from Lua table.");
        return luaL_error(L, "Unable to create json object from Lua table.");
    }

    const char *str = json_object_to_json_string_ext(obj, flags);
    if (!str) {
        const char *err = json_util_get_last_err();
        LOG("Unable to convert json object to string: %s", err);
        return luaL_error(L, "json-serialize: %s", err);
    }
    lua_pushstring(L, str);

    json_object_put(obj);

    return 1;
}

int l_module_json_deserialize(lua_State *L) {
    int s = selfshift(L);
    const char *str = luaL_checkstring(L, s);

    json_object *obj = json_tokener_parse(str);
    if (!obj) {
        const char *err = json_util_get_last_err();
        LOG("json-deserialize: %s", err);
        return luaL_error(L, "json-deserialize: %s", err);
    }

    json_to_lua(L, obj);

    json_object_put(obj);

    return 1;
}

/*----------- registration ------------------------------------------*/
static const luaL_Reg module_fns[] = {
    {"serialize", l_module_json_serialize},     //
    {"deserialize", l_module_json_deserialize}, //
    {NULL, NULL},                               //
};

int l_module_json_register_module(lua_State *L) {
    LOG("Registering taf-json module...");

    LOG("Registering module functions...");
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    LOG("Module functions registered.");

    LOG("Successfully registered taf-json module.");
    return 1;
}
