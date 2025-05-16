#include "modules/taf.h"

#include "util/time.h"

/* taf:millis() -> number */
int l_module_taf_millis(lua_State *L) {
    uint64_t uptime = millis_since_start();
    lua_pushnumber(L, uptime);
    return 1;
}

static int l_gc(lua_State *) { return 0; }

static const luaL_Reg port_mt[] = {
    {"__gc", l_gc}, //
    {NULL, NULL},   //
};

static const luaL_Reg module_fns[] = {
    {"millis", l_module_taf_millis},
    {NULL, NULL},
};

int l_module_taf_register_module(lua_State *L) {

    luaL_newmetatable(L, "taf");
    luaL_setfuncs(L, port_mt, 0);
    lua_pop(L, 1);
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    return 1;
}
