#include "modules/taf-serial.h"
#include "modules/taf.h"
#include "test_case.h"
#include "version.h"

#include <notcurses/notcurses.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int c_sleep_ms(lua_State *L) {
    int ms = luaL_checkinteger(L, 1);
    usleep(ms * 1000);

    return 0; /* no Lua return values */
}

static int l_register_test(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushvalue(L, 2);                      /* duplicate fn -> top */
    int ref = luaL_ref(L, LUA_REGISTRYINDEX); /* pop & ref */

    test_case_t test_case = {.name = name, .ref = ref};
    test_case_enqueue(&test_case);

    return 0; /* no Lua returns */
}

static int run_all_tests(lua_State *L) {
    size_t passed = 0;

    size_t amount;
    test_case_t *tests = test_case_get_all(&amount);

    for (size_t i = 0; i < amount; ++i) {
        printf("▶ %s … ", tests[i].name);
        fflush(stdout);

        lua_rawgeti(L, LUA_REGISTRYINDEX, tests[i].ref); /* push fn */
        int rc = lua_pcall(L, 0, 0, 0);
        if (rc == LUA_OK) {
            printf("PASS\n");
            passed++;
        } else {
            printf("FAIL\n");
            fprintf(stderr, "    %s\n", lua_tostring(L, -1));
            lua_pop(L, 1); /* pop err msg */
        }
    }

    printf("\nSummary: %zu / %zu passed\n", passed, amount);
    return passed == amount ? EXIT_SUCCESS : EXIT_FAILURE;
}

static const char *get_lib_dir() {

    // TODO: check optional CMD arg first

    // Check 'TAF_LIB_PATH' env variable
    const char *env_path = getenv("TAF_LIB_PATH");
    if (env_path && *env_path) {
        fprintf(stdout, "TAF_LIB_PATH is set.\n");
        fprintf(stdout, "Loading taf lua library at path '%s'\n", env_path);

        return env_path;
    }

    // Check default fallback
    const char *home_path = getenv("HOME");
    if (!home_path || !*home_path) {
        fprintf(stderr, "Unable to load taf lua library: 'HOME' "
                        "environment variable is not set.\n"
                        "Use --libpath path_to_taf_lib_folder or set "
                        "'taf_LIB_PATH' environment variable.\n");
        exit(EXIT_FAILURE);
    }
    char default_path[PATH_MAX];
    snprintf(default_path, sizeof default_path, "%s/.taf/" TAF_VERSION "/lib",
             home_path);
    fprintf(stdout, "Loading taf lua library at path '%s'\n", default_path);

    return strdup(default_path);
}

static void inject_modules_dir(lua_State *L) {

    const char *module_path = get_lib_dir();

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path"); /* pkg.path string */
    const char *old_path = lua_tostring(L, -1);
    lua_pushfstring(L, "%s;%s/?.lua;%s/?/init.lua", old_path, module_path,
                    module_path);
    lua_setfield(L, -3, "path"); /* package.path = … */
    lua_pop(L, 2);               /* pop path + package */
}

void register_test_api(lua_State *L) {
    /* ---- helpers, test_case, … --------------------------------- */
    lua_newtable(L);
    lua_pushcfunction(L, c_sleep_ms);
    lua_setfield(L, -2, "sleep");
    lua_setglobal(L, "test");

    lua_pushcfunction(L, l_register_test);
    lua_setglobal(L, "test_case");

    /* ---- make C serial module visible to require() -------------- */
    /* pushes the module table, sets package.loaded["serial"], and   */
    /* stores the C function in package.preload for future require() */
    luaL_requiref(L, "taf-serial", l_module_serial_register_module, 1);
    lua_pop(L, 1); /* remove the module table we just required */
    luaL_requiref(L, "taf", l_module_taf_register_module, 1);
    lua_pop(L, 1); /* remove the module table we just required */

    inject_modules_dir(L);
}

int main(int argc, char **argv) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    register_test_api(L);

    /* -------- load every file on CLI ---------------- */
    for (int i = 1; i < argc; ++i) {
        if (luaL_dofile(L, argv[i])) {
            fprintf(stderr, "[ERROR] Lua error loading %s: %s\n", argv[i],
                    lua_tostring(L, -1));
            lua_pop(L, 1);
            return EXIT_FAILURE;
        }
    }

    /* -------- run the queued tests ------------------ */
    int exitcode = run_all_tests(L);

    /* -------- tidy‑up ------------------------------- */
    test_case_free_all(L);

    lua_close(L);

    return exitcode;
}
