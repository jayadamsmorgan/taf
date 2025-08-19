#include "modules/taf/taf.h"

#include "cmd_parser.h"
#include "internal_logging.h"
#include "taf_test.h"
#include "taf_vars.h"
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

static void read_string_array(lua_State *L, int idx, char ***out,
                              size_t *out_n) {
    luaL_checktype(L, idx, LUA_TTABLE);
    lua_Integer n = luaL_len(L, idx);
    if (n < 0)
        n = 0;

    char **arr = malloc(n * sizeof(char *));

    for (lua_Integer i = 1; i <= n; ++i) {
        lua_rawgeti(L, idx, i);
        const char *s = luaL_checkstring(L, -1);
        arr[i - 1] = strdup(s);
        lua_pop(L, 1);
    }

    *out = arr;
    *out_n = (size_t)n;
}

int l_module_taf_register_test(lua_State *L) {
    LOG("Registering new test...");

    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "name");
    const char *name = luaL_checkstring(L, -1);
    char *name_copy = strdup(name);
    lua_pop(L, 1);

    lua_getfield(L, 1, "body");
    luaL_argcheck(L, lua_isfunction(L, -1), 1, "`body` must be a function");
    int body_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    test_case_t *tc = malloc(sizeof(test_case_t));
    memset(tc, 0, sizeof(*tc));
    tc->ref = body_ref;
    tc->name = name_copy;

    lua_getfield(L, 1, "description");
    if (!lua_isnil(L, -1)) {
        luaL_argcheck(L, lua_isstring(L, -1), 1,
                      "`description` must be string");
        tc->desc = strdup(lua_tostring(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "tags");
    if (!lua_isnil(L, -1)) {
        luaL_argcheck(L, lua_istable(L, -1), 1,
                      "`tags` must be array of strings");
        read_string_array(L, lua_gettop(L), &tc->tags.tags, &tc->tags.amount);
    }
    cmd_test_options *opts = cmd_parser_get_test_options();
    if (opts->tags_amount > 0) {
        bool has_tag = false;
        for (size_t i = 0; i < opts->tags_amount; i++) {
            for (size_t j = 0; j < tc->tags.amount; j++) {
                if (strcmp(opts->tags[i], tc->tags.tags[j]) == 0) {
                    has_tag = true;
                    break;
                }
            }
            if (has_tag)
                break;
        }
        if (!has_tag) {
            LOG("Skipping test '%s', no tag found.", tc->name);
            // TODO: Fix leak
            return 0;
        }
    }
    lua_pop(L, 1);

    test_case_enqueue(tc);

    LOG("Successfully registered new test %s", tc->name);

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

int l_module_taf_get_active_tags(lua_State *L) {
    LOG("Getting active tags...");

    cmd_test_options *opts = cmd_parser_get_test_options();

    lua_newtable(L);
    for (size_t i = 0; i < opts->tags_amount; i++) {
        lua_pushstring(L, opts->tags[i]);
        lua_rawseti(L, -2, i + 1);
    }

    LOG("Successfully got active tags.");

    return 1;
}

int l_module_taf_get_active_test_tags(lua_State *L) {
    LOG("Getting active test tags...");

    cmd_test_options *opts = cmd_parser_get_test_options();
    test_case_t *test = taf_test_get_current_test();

    lua_newtable(L);
    size_t index = 1;
    for (size_t i = 0; i < opts->tags_amount; i++) {
        for (size_t j = 0; j < test->tags.amount; j++) {
            if (strcmp(opts->tags[i], test->tags.tags[j]) == 0) {
                LOG("Active test tag: %s", opts->tags[i]);
                lua_pushstring(L, opts->tags[i]);
                lua_rawseti(L, -2, index++);
                break;
            }
        }
    }

    LOG("Successfully got active test tags.");

    return 1;
}

int l_module_taf_register_vars(lua_State *L) {
    LOG("Registering TAF variables...");

    luaL_checktype(L, 1, LUA_TTABLE);

    size_t count = 0;
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        lua_pop(L, 1);
        ++count;
    }

    taf_var_map_t *map = malloc(sizeof(taf_var_map_t));
    memset(map, 0, sizeof(*map));
    map->count = count;
    if (count > 0) {
        map->entries = malloc(count * sizeof(taf_var_entry_t));
        memset(map->entries, 0, count * sizeof(taf_var_entry_t));
    }

    size_t idx = 0;
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        if (!lua_isstring(L, -2)) {
            lua_pop(L, 1);
            luaL_error(L, "taf_vars keys must be strings");
        }

        taf_var_entry_t *e = &map->entries[idx];
        memset(e, 0, sizeof(*e));
        e->name = strdup(lua_tostring(L, -2));

        if (lua_type(L, -1) == LUA_TSTRING) {
            e->is_scalar = true;
            e->scalar = strdup(lua_tostring(L, -1));
        } else if (lua_type(L, -1) == LUA_TTABLE) {
            e->is_scalar = false;

            lua_getfield(L, -1, "values");
            if (!lua_isnil(L, -1)) {
                luaL_argcheck(L, lua_istable(L, -1), 1,
                              "`values` must be array of strings");
                read_string_array(L, lua_gettop(L), &e->values,
                                  &e->values_amount);
            }
            lua_pop(L, 1);

            lua_getfield(L, -1, "default");
            if (!lua_isnil(L, -1)) {
                luaL_argcheck(L, lua_isstring(L, -1), 1,
                              "`default` must be string");
                e->def_value = strdup(lua_tostring(L, -1));
                bool found = false;
                for (size_t i = 0; i < e->values_amount; i++) {
                    if (strcmp(e->def_value, e->values[i]) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found && e->values_amount > 0) {
                    lua_pop(L, 1);
                    luaL_error(L,
                               "Error parsing var '%s': Default value "
                               "'%s' is not an allowed value.",
                               e->name, e->def_value);
                }
            }
            lua_pop(L, 1);
        } else {
            lua_pop(L, 1);
            luaL_error(L,
                       "Error parsing value for var '%s': value must be string "
                       "or table",
                       e->name);
        }

        lua_pop(L, 1);
        ++idx;
    }

    taf_register_vars(map);

    return 0;
}

int l_module_taf_get_var(lua_State *L) {
    LOG("Getting var...");

    int s = selfshift(L);
    const char *var_name = luaL_checkstring(L, s);

    LOG("Variable name: '%s'", var_name);

    taf_var_map_t *map = taf_get_vars();
    for (size_t i = 0; i < map->count; i++) {
        if (strcmp(var_name, map->entries[i].name) == 0) {
            if (!map->entries[i].final_value) {
                luaL_error(
                    L,
                    "Unknown error occured, cannot get value for variable '%s'",
                    var_name);
                return 0;
            }
            lua_pushstring(L, map->entries[i].final_value);
            return 1;
        }
    }

    LOG("No variable '%s'", var_name);
    luaL_error(L, "No variable '%s'", var_name);
    return 0;
}

int l_module_taf_get_vars(lua_State *L) {
    LOG("Getting all variables...");

    lua_newtable(L);

    taf_var_map_t *map = taf_get_vars();
    for (size_t i = 0; i < map->count; i++) {
        const char *name = map->entries[i].name;
        const char *value = map->entries[i].final_value;

        if (!value) {
            luaL_error(
                L, "Unknown error occured, cannot get value for variable '%s'",
                name);
            return 0;
        }
        lua_pushstring(L, value);
        lua_setfield(L, -2, name);
    }

    return 1;
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

    LOG("Final message string: %.*s", (int)mlen, copy);

    const char *file = "(?)";
    int line = 0;
    lua_Debug ar;

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

    taf_log_test(level, file, line, copy, mlen);

    if (level == TAF_LOG_LEVEL_CRITICAL) {
        LOG("Log level is critical, raising error...");
        luaL_error(L, "%s", copy);
        free(copy); // Doesn't get called so the `copy` will leak,
                    // not so much we can do about it
        return;
    }
    free(copy);

    lua_pop(L, 1);
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
    {"defer", l_module_taf_defer},                               //
    {"get_active_tags", l_module_taf_get_active_tags},           //
    {"get_active_test_tags", l_module_taf_get_active_test_tags}, //
    {"get_current_target", l_module_taf_get_current_target},     //
    {"get_var", l_module_taf_get_var},                           //
    {"get_vars", l_module_taf_get_vars},                         //
    {"sleep", l_module_taf_sleep},                               //
    {"millis", l_module_taf_millis},                             //
    {"print", l_module_taf_print},                               //
    {"log", l_module_taf_log},                                   //
    {"test", l_module_taf_register_test},                        //
    {"register_vars", l_module_taf_register_vars},               //
    {NULL, NULL},                                                //
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
