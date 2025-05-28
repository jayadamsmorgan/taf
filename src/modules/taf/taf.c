#include "modules/taf/taf.h"

#include "taf_tui.h"
#include "test_case.h"
#include "util/lua.h"
#include "util/time.h"

#include <stdlib.h>
#include <string.h>

/* taf:sleep(ms: number) */
static int l_module_taf_sleep(lua_State *L) {
    int s = selfshift(L);

    int ms = luaL_checkinteger(L, s);
    usleep(ms * 1000);

    return 0; /* no Lua return values */
}

/* taf:test(name:string, body:function) */
static int l_module_taf_register_test(lua_State *L) {
    int s = selfshift(L);

    const char *name = luaL_checkstring(L, s);
    luaL_checktype(L, s + 1, LUA_TFUNCTION);

    lua_pushvalue(L, s + 1);                  /* duplicate fn -> top */
    int ref = luaL_ref(L, LUA_REGISTRYINDEX); /* pop & ref */

    test_case_t test_case = {.name = name, .ref = ref};
    test_case_enqueue(&test_case);

    return 0; /* no Lua returns */
}

/* taf:millis() -> number */
int l_module_taf_millis(lua_State *L) {
    uint64_t uptime = millis_since_start();
    lua_pushnumber(L, uptime);
    return 1;
}

/* taf:print() */
int l_module_taf_print(lua_State *L) {
    int n = lua_gettop(L); /* #args */

    int s = selfshift(L);

    luaL_Buffer buf;
    luaL_buffinit(L, &buf);

    for (int i = s; i <= n; ++i) {
        size_t len;
        const char *s = luaL_tolstring(L, i, &len);
        luaL_addlstring(&buf, s, len); /* copy into buffer */
        lua_pop(L, 1);                 /* pop the string   */

        if (i < n)
            luaL_addchar(&buf, '\t');
    }
    luaL_pushresult(&buf); /* message string on top */

    const char *file = "(?)";
    int line = 0;
    lua_Debug ar;

    if (lua_getstack(L, 1, &ar) && lua_getinfo(L, "Sl", &ar) &&
        ar.currentline > 0) {
        file = ar.short_src;
        line = ar.currentline;
    }

    const char *msg = lua_tostring(L, -1); /* still valid on stack */
    taf_tui_log(file, line, msg);

    lua_pop(L, 1); /* pop message string */
    return 0;
}

static int l_gc(lua_State *) { return 0; }

static const luaL_Reg port_mt[] = {
    {"__gc", l_gc}, //
    {NULL, NULL},   //
};

static const luaL_Reg module_fns[] = {
    {"sleep", l_module_taf_sleep},
    {"millis", l_module_taf_millis},
    {"print", l_module_taf_print},
    {"test", l_module_taf_register_test},
    {NULL, NULL},
};

int l_module_taf_register_module(lua_State *L) {

    luaL_newmetatable(L, "taf-main");
    luaL_setfuncs(L, port_mt, 0);
    lua_pop(L, 1);
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    return 1;
}
