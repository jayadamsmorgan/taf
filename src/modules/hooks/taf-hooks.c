#include "modules/hooks/taf-hooks.h"

#include "internal_logging.h"
#include "taf_hooks.h"

#include "util/lua.h"

static int register_hook(lua_State *L, taf_hook_fn fn) {
    LOG("Registering hook with type %d...", fn);

    int s = selfshift(L);

    luaL_checktype(L, s, LUA_TFUNCTION);
    lua_pushvalue(L, s);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    taf_hook_t hook = {
        .ref = ref,
        .fn = fn,
    };

    taf_hooks_add_to_queue(hook);

    LOG("Successfully registered hook with type %d.", fn);

    return 0;
}

int l_module_hooks_register_test_run_started(lua_State *L) {
    return register_hook(L, TAF_HOOK_FN_TEST_RUN_STARTED);
}

int l_module_hooks_register_test_started(lua_State *L) {
    return register_hook(L, TAF_HOOK_FN_TEST_STARTED);
}

int l_module_hooks_register_test_finished(lua_State *L) {
    return register_hook(L, TAF_HOOK_FN_TEST_FINISHED);
}

int l_module_hooks_register_test_run_finished(lua_State *L) {
    return register_hook(L, TAF_HOOK_FN_TEST_RUN_FINISHED);
}

/*----------- registration ------------------------------------------*/
static const luaL_Reg module_fns[] = {
    {"register_test_run_started", l_module_hooks_register_test_run_started}, //
    {"register_test_started", l_module_hooks_register_test_started},         //
    {"register_test_finished", l_module_hooks_register_test_finished},       //
    {"register_test_run_finished",
     l_module_hooks_register_test_run_finished}, //
    {NULL, NULL},                                //
};

int l_module_hooks_register_module(lua_State *L) {
    LOG("Registering taf-hooks module...");

    LOG("Registering module functions...");
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    LOG("Module functions registered.");

    LOG("Successfully registere taf-hooks module.");
    return 1;
}
