#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <unistd.h>

#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------
 * Test‑case queue (grow‑able array of {name, Lua registry reference})
 * ------------------------------------------------------------------*/
typedef struct {
    char *name; /* strdup‑ed test name   */
    int ref;    /* reference to Lua fn   */
} TestCase;

static TestCase *tests = NULL;
static size_t tests_len = 0;
static size_t tests_cap = 0;

static void queue_test(const char *name, int ref) {
    if (tests_len == tests_cap) { /* grow 2× */
        tests_cap = tests_cap ? tests_cap * 2 : 8;
        tests = realloc(tests, tests_cap * sizeof(TestCase));
        if (!tests) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
    }
    tests[tests_len].name = strdup(name);
    tests[tests_len].ref = ref;
    tests_len++;
}

static int c_sleep_ms(lua_State *L) {
    int ms = luaL_checkinteger(L, 1);
    usleep(ms * 1000);
    printf("SLEEEP!!\n");

    return 0; /* no Lua return values */
}

static int l_register_test(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushvalue(L, 2);                      /* duplicate fn -> top */
    int ref = luaL_ref(L, LUA_REGISTRYINDEX); /* pop & ref */

    queue_test(name, ref); /* remember it */

    return 0; /* no Lua returns */
}

static int run_all_tests(lua_State *L) {
    size_t passed = 0;

    for (size_t i = 0; i < tests_len; ++i) {
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

    printf("\nSummary: %zu / %zu passed\n", passed, tests_len);
    return passed == tests_len ? EXIT_SUCCESS : EXIT_FAILURE;
}

void register_test_api(lua_State *L) {
    lua_newtable(L); /* test */
    lua_pushcfunction(L, c_sleep_ms);
    lua_setfield(L, -2, "sleep");
    /* …more helpers… */
    lua_setglobal(L, "test"); /* global "test" table */

    lua_pushcfunction(L, l_register_test);
    lua_setglobal(L, "test_case");
}

int main(int argc, char **argv) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    register_test_api(L);

    /* -------- load every file on CLI ---------------- */
    for (int i = 1; i < argc; ++i) {
        if (luaL_dofile(L, argv[i])) {
            fprintf(stderr, "Lua error loading %s: %s\n", argv[i],
                    lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }

    /* -------- run the queued tests ------------------ */
    int exitcode = run_all_tests(L);

    /* -------- tidy‑up ------------------------------- */
    for (size_t i = 0; i < tests_len; ++i) {
        luaL_unref(L, LUA_REGISTRYINDEX, tests[i].ref);
        free(tests[i].name);
    }
    free(tests);
    lua_close(L);

    return exitcode;
}
