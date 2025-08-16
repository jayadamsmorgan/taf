#include "taf_hooks.h"

#include "taf_tui.h"

#include "cmd_parser.h"
#include "headless.h"
#include "internal_logging.h"

#include "util/time.h"

#include <stdlib.h>

typedef struct {
    taf_hook_t *hooks;
    size_t count;
    size_t capacity;
} hook_da_t;

static hook_da_t hooks_test_run_started = {
    .hooks = NULL,
    .capacity = 2,
    .count = 0,
};
static hook_da_t hooks_test_started = {
    .hooks = NULL,
    .capacity = 2,
    .count = 0,
};
static hook_da_t hooks_test_finished = {
    .hooks = NULL,
    .capacity = 2,
    .count = 0,
};
static hook_da_t hooks_test_run_finished = {
    .hooks = NULL,
    .capacity = 2,
    .count = 0,
};

static bool headless = false;

static inline hook_da_t *taf_get_hooks(taf_hook_fn fn) {
    switch (fn) {
    case TAF_HOOK_FN_TEST_RUN_STARTED:
        return &hooks_test_run_started;
    case TAF_HOOK_FN_TEST_STARTED:
        return &hooks_test_started;
    case TAF_HOOK_FN_TEST_FINISHED:
        return &hooks_test_finished;
    case TAF_HOOK_FN_TEST_RUN_FINISHED:
        return &hooks_test_run_finished;
    }
    return NULL;
}

static void hook_da_append(hook_da_t *hook_da, taf_hook_t hook) {
    if (!hook_da->hooks) {
        hook_da->hooks = malloc(sizeof(taf_hook_t) * hook_da->capacity);
    }
    if (hook_da->count >= hook_da->capacity) {
        hook_da->capacity *= 2;
        hook_da->hooks =
            realloc(hook_da->hooks, sizeof(taf_hook_t) * hook_da->capacity);
    }
    hook_da->hooks[hook_da->count] = hook;
    hook_da->count++;
}

void taf_hooks_init() {
    LOG("Initializing TAF hooks...");

    cmd_test_options *opts = cmd_parser_get_test_options();
    headless = opts->headless;

    LOG("Successfully initialized TAF hooks.");
}

void taf_hooks_add_to_queue(taf_hook_t hook) {
    LOG("Adding hook with type %d and ref %d", hook.fn, hook.ref);
    hook_da_t *hooks_da = taf_get_hooks(hook.fn);
    hook_da_append(hooks_da, hook);
    LOG("Successfully added hook.");
}

void taf_hooks_run(lua_State *L, taf_hook_fn fn,
                   int (*context_push)(lua_State *L)) {
    LOG("Running all hooks with type %d...", fn);
    hook_da_t *hooks = taf_get_hooks(fn);
    if (hooks->count == 0) {
        LOG("No hooks found for type %d", fn);
        return;
    }
    for (size_t i = 0; i < hooks->count; i++) {
        int ref = hooks->hooks[i].ref;
        LOG("Running hook with type %d and ref %d", fn, ref);
        lua_rawgeti(L, LUA_REGISTRYINDEX, hooks->hooks[i].ref);
        context_push(L);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            const char *err = lua_tostring(L, -1);
            LOG("Error running hook with type %d and ref %d:\n%s", fn, ref,
                err);
            if (headless) {
                taf_headless_hook_failed(err);
            } else {
                char ts[TS_LEN];
                get_date_time_now(ts);
                taf_tui_hook_failed(ts, err);
            }
            continue;
        }
        LOG("Successfully ran hook with type %d and ref %d", fn, ref);
    }
    LOG("Successfully ran all hooks with type %d.", fn);
}

void taf_hooks_deinit() {
    LOG("Deinitializing TAF hooks...");
    free(hooks_test_run_started.hooks);
    free(hooks_test_started.hooks);
    free(hooks_test_finished.hooks);
    free(hooks_test_run_finished.hooks);
    LOG("Successfully deinitialized TAF hooks.");
}
