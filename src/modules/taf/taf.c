#include "modules/taf/taf.h"

#include "cmd_parser.h"
#include "test_case.h"
#include "test_logs.h"
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
    int body_index;

    cmd_test_options *opts = cmd_parser_get_test_options();
    bool register_test = false;
    if (opts->tags_amount == 0) {
        register_test = true;
    }

    test_tags_t tags = {.tags = NULL, .amount = 0};

    switch (lua_type(L, s + 1)) {
    case LUA_TFUNCTION: {
        // Second arg is test body, test has no tags, skipping third arg
        body_index = 1;
        break;
    }
    case LUA_TTABLE: {
        // Got tags, check them & test body
        lua_Integer max;
        int is_array = lua_table_is_array(L, s + 1, &max);
        if (!is_array) {
            luaL_error(L, "Tags are supposed to be an array of strings, e.g.: "
                          "{\"tag1\", \"tag2\"}");
            return 0;
        }

        tags.amount = max;
        tags.tags = malloc(sizeof(*tags.tags) * tags.amount);

        for (lua_Integer i = 1; i <= max; ++i) {
            lua_rawgeti(L, s + 1, i); // push tags[i]

            if (!lua_isstring(L, -1)) {
                return luaL_error(L, "Tag #%lld is not a string (got %s)",
                                  (long long)i, luaL_typename(L, -1));
            }

            const char *tag = lua_tostring(L, -1);
            tags.tags[i - 1] = strdup(tag);

            for (size_t i = 0; i < opts->tags_amount; i++) {
                if (!strcmp(tag, opts->tags[i])) {
                    register_test = true;
                }
            }

            lua_pop(L, 1); // pop tags[i]
        }

        body_index = 2;
        luaL_checktype(L, s + 2, LUA_TFUNCTION);
        break;
    }
    default: {
        luaL_error(
            L,
            "Expected test body or array of tags for second argument, got %s.",
            lua_typename(L, s + 1));
        return 0;
    }
    }

    if (!register_test) {
        // Skipping this test, does not contain required tags
        return 0;
    }

    lua_pushvalue(L, s + body_index);         /* duplicate fn -> top */
    int ref = luaL_ref(L, LUA_REGISTRYINDEX); /* pop & ref */

    test_case_t test_case = {.name = name, .tags = tags, .ref = ref};
    test_case_enqueue(&test_case);

    return 0;
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
        if (ar.source[0] == '@') {
            file = &ar.source[1];
        } else {
            file = ar.source;
        }
        line = ar.currentline;
    }

    const char *msg = lua_tostring(L, -1); /* still valid on stack */
    taf_log_test(file, line, msg);

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
