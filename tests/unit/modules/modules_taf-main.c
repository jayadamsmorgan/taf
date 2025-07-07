#include "modules/taf/taf.h"

#include "util/time.h"

#include <criterion/criterion.h>
#include <criterion/new/assert.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <time.h>
#include <unistd.h>

static lua_State *L = NULL;

static void run_lua(const char *chunk) {
    int rc = luaL_dostring(L, chunk);
    if (rc != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        cr_log_error("Lua error: %s", msg ? msg : "(non-string error)");
        cr_fail();
    }
}

static void taf_setup(void) {
    L = luaL_newstate();
    cr_assert_not_null(L);
    luaL_openlibs(L);
    l_module_taf_register_module(L);
    lua_setglobal(L, "taf");
}

static void taf_teardown(void) {
    lua_close(L);
    L = NULL;
}

TestSuite(taf_module, .init = taf_setup, .fini = taf_teardown);

/* --------------------------------------------------------------------------
 *  1.  taf.sleep() waits roughly the requested time
 * ------------------------------------------------------------------------*/
Test(taf_module, sleep_waits) {
    unsigned long t0 = millis_since_start();

    run_lua("taf.sleep(10)");

    unsigned long elapsed = millis_since_start() - t0;

    cr_expect(elapsed >= 10);
}

/* --------------------------------------------------------------------------
 *  2.  taf.defer() populates the internal queue
 * ------------------------------------------------------------------------*/
Test(taf_module, defer_enqueues_one) {
    /* defer(function() end) */
    run_lua("taf.defer(function() return 42 end)");

    lua_getfield(L, LUA_REGISTRYINDEX, DEFER_LIST_KEY);
    lua_Integer len = luaL_len(L, -1);
    cr_expect_eq(len, 1);
    lua_pop(L, 1); /* pop queue */
}

/* --------------------------------------------------------------------------
 *  4.  taf.test() registers a test in C queue (smoke-test)
 * ------------------------------------------------------------------------*/
Test(taf_module, register_test_smoke) {
    // We don’t peek into the internal queue here—just ensure no error.
    run_lua("taf.test(\"example\", function()\n"
            "-- empty body\n"
            "end)\n");
}
