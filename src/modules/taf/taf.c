#include "modules/taf/taf.h"

#include "cmd_parser.h"
#include "internal_logging.h"
#include "taf_test.h"
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

static void read_values_array(lua_State *L, int idx, const char ***out,
                              size_t *out_n) {
    luaL_checktype(L, idx, LUA_TTABLE);

    lua_Integer n = luaL_len(L, idx);
    if (n < 0)
        n = 0;

    const char **vals = NULL;
    size_t count = (size_t)n;
    if (count > 0) {
        vals = malloc(count * sizeof(const char *));
        for (lua_Integer i = 1; i <= n; ++i) {
            lua_rawgeti(L, idx, i);
            const char *s = luaL_checkstring(L, -1);
            vals[i - 1] = strdup(s);
            lua_pop(L, 1);
        }
    }

    *out = vals;
    *out_n = count;
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

    lua_getfield(L, 1, "vars");
    if (!lua_isnil(L, -1)) {
        luaL_argcheck(L, lua_istable(L, -1), 1, "`vars` must be a table");

        size_t count = 0;
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            lua_pop(L, 1);
            ++count;
        }

        if (count > 0) {
            tc->vars.var = malloc(count * sizeof(test_var_t));
            memset(tc->vars.var, 0, count * sizeof(test_var_t));
            tc->vars.var_amount = count;

            size_t idx = 0;
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                luaL_argcheck(L, lua_isstring(L, -2), 1,
                              "`vars` key must be string");
                luaL_argcheck(L, lua_istable(L, -1), 1,
                              "`vars` entry must be table");

                test_var_t *v = &tc->vars.var[idx];
                memset(v, 0, sizeof(*v));

                v->name = strdup(lua_tostring(L, -2));

                lua_getfield(L, -1, "default");
                if (!lua_isnil(L, -1)) {
                    luaL_argcheck(L, lua_isstring(L, -1), 1,
                                  "`default` must be string");
                    v->default_value = strdup(lua_tostring(L, -1));
                }
                lua_pop(L, 1);

                lua_Integer n = luaL_len(L, -1);
                if (n == 0) {
                    v->any_value = true;
                } else {
                    read_values_array(L, lua_gettop(L), &v->values,
                                      &v->values_amount);
                }

                lua_pop(L, 1);
                ++idx;
            }
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

int l_module_taf_get_var(lua_State *L) {
    LOG("Getting var...");

    int s = selfshift(L);
    const char *var_name = luaL_checkstring(L, s);

    LOG("Getting var '%s'", var_name);

    cmd_test_options *opts = cmd_parser_get_test_options();

    test_case_t *test = taf_test_get_current_test();
    if (test->vars.var_amount == 0) {
        luaL_error(L, "Var '%s' was not found for test '%s'", var_name,
                   test->name);
        return 0;
    }

    for (size_t i = 0; i < test->vars.var_amount; i++) {
        test_var_t *var = &test->vars.var[i];
        if (strcmp(var->name, var_name) == 0) {
            LOG("Found var in test declaration");
            cmd_var_t *found = NULL;
            for (size_t j = 0; j < opts->vars.count; j++) {
                if (strcmp(opts->vars.args[j].name, var->name) == 0) {
                    found = &opts->vars.args[j];
                    break;
                }
            }
            if (!found && !var->default_value) {
                luaL_error(L,
                           "No default value for variable '%s' specified. "
                           "Use '-v %s=value' or add default.",
                           var_name, var_name);
                return 0;
            }
            if (!found && var->default_value) {
                LOG("Using default value for variable %s", var_name);
                lua_pushstring(L, var->default_value);
                return 1;
            }
            if (var->any_value) {
                lua_pushstring(L, found->value);
                return 1;
            }

            for (size_t j = 0; j < var->values_amount; j++) {
                if (strcmp(var->values[j], found->value) == 0) {
                    LOG("Found value for variable %s", var->name);
                    lua_pushstring(L, found->value);
                    return 1;
                }
            }

            luaL_error(L, "'%s' is not a declared value in variable '%s'",
                       found->value, var_name);
            return 0;
        }
    }

    LOG("Have not found variable %s", var_name);
    luaL_error(L, "Have not found variable %s in test declaration", var_name);
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
    {"sleep", l_module_taf_sleep},                               //
    {"millis", l_module_taf_millis},                             //
    {"print", l_module_taf_print},                               //
    {"log", l_module_taf_log},                                   //
    {"test", l_module_taf_register_test},                        //
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
