#ifndef TAF_HOOKS_H
#define TAF_HOOKS_H

#include "taf_state.h"

typedef enum {
    TAF_HOOK_FN_TEST_RUN_STARTED = 0,
    TAF_HOOK_FN_TEST_STARTED = 1,
    TAF_HOOK_FN_TEST_FINISHED = 2,
    TAF_HOOK_FN_TEST_RUN_FINISHED = 3,
} taf_hook_fn;

typedef struct {
    int ref;
    taf_hook_fn fn;
} taf_hook_t;

void taf_hooks_init(taf_state_t *state);

void taf_hooks_add_to_queue(taf_hook_t hook);

void taf_hooks_run(lua_State *L, taf_hook_fn fn);

typedef void (*hook_cb)(taf_hook_fn);
typedef void (*hook_err_cb)(taf_hook_fn, const char *trace);

void taf_hooks_register_hook_started_cb(hook_cb cb);
void taf_hooks_register_hook_finished_cb(hook_cb cb);
void taf_hooks_register_hook_failed_cb(hook_err_cb cb);

void taf_hooks_deinit(lua_State *L);

#endif // TAF_HOOKS_H
