#include "modules/taf/taf.h"

#include "cmd_parser.h"
#include "internal_logging.h"
#include "test_case.h"
#include "test_logs.h"
#include "util/lua.h"
#include "util/time.h"

#include <stdlib.h>
#include <string.h>

int l_module_taf_sleep(lua_State *L) {
    LOG("Invoked taf-main sleep...");

    int s = selfshift(L);

    int ms = luaL_checkinteger(L, s);
    if (ms <= 0) {
        LOG("taf-main sleep ms %d <= 0", ms);
        return 0;
    }
    LOG("Sleeping for %d ms...", ms);
    usleep(ms * 1000);

    LOG("Successfully finished taf-main sleep");

    return 0; /* no Lua return values */
}

int l_module_taf_register_test(lua_State *L) {
    LOG("Registering new test...");

    int s = selfshift(L);

    const char *name = luaL_checkstring(L, s);
    LOG("Test name: '%s'", name);
    int body_index;

    cmd_test_options *opts = cmd_parser_get_test_options();
    bool register_test = false;
    if (opts->tags_amount == 0) {
        register_test = true;
    }

    test_tags_t tags = {.tags = NULL, .amount = 0};

    switch (lua_type(L, s + 1)) {
    case LUA_TFUNCTION: {
        LOG("Second argument is test body, skipping third argument...");
        body_index = 1;
        break;
    }
    case LUA_TTABLE: {
        // Got tags, check them & test body
        lua_Integer max;
        int is_array = lua_table_is_array(L, s + 1, &max);
        if (!is_array) {
            LOG("Second argument is table but not an array, throwing error...");
            luaL_error(L, "Tags are supposed to be an array of strings, e.g.: "
                          "{\"tag1\", \"tag2\"}");
            return 0;
        }

        tags.amount = max;
        tags.tags = malloc(sizeof(*tags.tags) * tags.amount);

        for (lua_Integer i = 1; i <= max; ++i) {
            lua_rawgeti(L, s + 1, i); // push tags[i]

            if (!lua_isstring(L, -1)) {
                LOG("One of the tags is not a string, throwing error...");
                luaL_error(L, "Tag #%lld is not a string (got %s)",
                           (long long)i, luaL_typename(L, -1));
                return 0;
            }

            const char *tag = lua_tostring(L, -1);
            LOG("Adding test tag %s", tag);
            tags.tags[i - 1] = strdup(tag);

            for (size_t i = 0; i < opts->tags_amount; i++) {
                if (!strcmp(tag, opts->tags[i])) {
                    LOG("Found tags match with test run.");
                    register_test = true;
                }
            }

            lua_pop(L, 1); // pop tags[i]
        }

        LOG("Successfully added test tags.");

        body_index = 2;
        luaL_checktype(L, s + 2, LUA_TFUNCTION);
        break;
    }
    default: {
        const char *typename = lua_typename(L, s + 1);
        LOG("Wrong argument type for second argument: %s", typename);
        luaL_error(
            L,
            "Expected test body or array of tags for second argument, got %s.",
            typename);
        return 0;
    }
    }

    if (!register_test) {
        LOG("Skipping test registering: test does not containg required tags.");
        return 0;
    }

    lua_pushvalue(L, s + body_index);         /* duplicate fn -> top */
    int ref = luaL_ref(L, LUA_REGISTRYINDEX); /* pop & ref */

    test_case_t test_case = {.name = strdup(name), .tags = tags, .ref = ref};

    LOG("Creating test case with name %s,  reference %d", test_case.name,
        test_case.ref);

    test_case_enqueue(&test_case);

    LOG("Successfully registered new test '%s'", test_case.name);

    return 0;
}

int l_module_taf_defer(lua_State *L) {
    LOG("Adding new defer to defer queue...");

    int s = selfshift(L);
    int nargs = lua_gettop(L) - (s - 1);
    LOG("Amount of arguments: %d", nargs);

    luaL_checktype(L, s, LUA_TFUNCTION);

    lua_getfield(L, LUA_REGISTRYINDEX, DEFER_LIST_KEY);
    if (lua_isnil(L, -1)) {
        LOG("Defer queue is nil, creating new one...");
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, DEFER_LIST_KEY);
        LOG("Successfully created new defer queue.");
    }
    int list = lua_gettop(L);

    LOG("Pushing defer to defer queue...");
    lua_newtable(L);
    for (int i = 0; i < nargs; ++i) {
        lua_pushvalue(L, s + i);
        lua_rawseti(L, -2, i + 1);
    }

    lua_Integer idx = luaL_len(L, list) + 1;
    lua_rawseti(L, list, idx);

    LOG("Successfully added new defer to defer queue.");

    return 0;
}

static inline void log_helper(taf_log_level level, int n, int s, lua_State *L) {
    LOG("Constructing log message buffer with %d arguments...", n);
    luaL_Buffer buf;
    luaL_buffinit(L, &buf);

    for (int i = s; i <= n; i++) {

        luaL_tolstring(L, i, NULL);

        luaL_addvalue(&buf);

        if (i < n) {
            luaL_addchar(&buf, '\t');
        }
    }
    luaL_pushresult(&buf);

    size_t mlen;
    const char *msg = lua_tolstring(L, -1, &mlen);

    char *copy = malloc(mlen + 1);
    if (!copy) {
        luaL_error(L, "out of memory");
        return;
    }
    memcpy(copy, msg, mlen);
    copy[mlen] = '\0';

    LOG("Final message string: %.*s", (int)mlen, copy); // Use copy for safety

    const char *file = "(?)";
    int line = 0;
    lua_Debug ar;

    // This logic seems fine.
    if (lua_getstack(L, 2, &ar) && lua_getinfo(L, "Sl", &ar) &&
        ar.currentline > 0) {
        LOG("Parent caller detected.");
        file = (ar.source[0] == '@') ? ar.source + 1 : ar.source;
        line = ar.currentline;
    } else if (lua_getstack(L, 1, &ar) && lua_getinfo(L, "Sl", &ar) &&
               ar.currentline > 0) {
        LOG("Direct caller detected.");
        file = (ar.source[0] == '@') ? ar.source + 1 : ar.source;
        line = ar.currentline;
    }
    LOG("File: %s, line: %d", file, line);

    if (level == TAF_LOG_LEVEL_CRITICAL) {
        LOG("Log level is critical, raising error...");
        // luaL_error performs a longjmp, so 'copy' would be leaked.
        // While hard to avoid, it's good to be aware of.
        // The error message is now safely formatted.
        luaL_error(L, "%s", copy);
        free(copy); // This line will not be reached, but is good practice.
        return;
    }

    // Now 'copy' is a proper C string and 'mlen' is its correct length (without
    // null).
    taf_log_test(level, file, line, copy, mlen);

    free(copy);

    lua_pop(L, 1); // pop message string
}

int l_module_taf_log(lua_State *L) {

    LOG("Invoked taf-main log...");

    int n = lua_gettop(L);

    int s = selfshift(L);

    const char *log_level_str = luaL_checkstring(L, s);

    int log_level = taf_log_level_from_str(log_level_str);
    if (log_level == -1) {
        LOG("Unknown log level %s, throwing error...", log_level_str);
        luaL_error(L, "Unknown log level '%s'", log_level_str);
        return 0;
    }

    log_helper(log_level, n, s + 1, L);

    LOG("Successfully finished taf-main log.");

    return 0;
}

int l_module_taf_print(lua_State *L) {

    LOG("Invoked taf-main print...");

    int n = lua_gettop(L);

    int s = selfshift(L);

    log_helper(TAF_LOG_LEVEL_INFO, n, s, L);

    LOG("Successfully finished taf-main print.");

    return 0;
}

int l_module_taf_millis(lua_State *L) {

    LOG("Invoked taf-main millis...");

    unsigned long uptime = millis_since_start();

    LOG("Pushing uptime: %lu", uptime);
    lua_pushnumber(L, uptime);

    LOG("Sucessfully finished taf-main uptime.");

    return 1;
}

int l_module_taf_get_current_target(lua_State *L) {

    LOG("Invokedd taf-main get_current_target...");

    cmd_test_options *opts = cmd_parser_get_test_options();
    if (opts->target) {
        lua_pushstring(L, opts->target);
    } else {
        lua_pushstring(L, "");
    }

    LOG("Successfully finished taf-main get_current_target.");

    return 1;
}

/*----------- registration ------------------------------------------*/
static const luaL_Reg module_fns[] = {
    {"defer", l_module_taf_defer},                           //
    {"get_current_target", l_module_taf_get_current_target}, //
    {"sleep", l_module_taf_sleep},                           //
    {"millis", l_module_taf_millis},                         //
    {"print", l_module_taf_print},                           //
    {"log", l_module_taf_log},                               //
    {"test", l_module_taf_register_test},                    //
    {NULL, NULL},                                            //
};

int l_module_taf_register_module(lua_State *L) {
    LOG("Registering taf-main module...");

    LOG("Registering module functions...");
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    LOG("Module functions registered.");

    LOG("Successfully registered taf-main module.");
    return 1;
}
