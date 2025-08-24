#ifndef TAF_HOOKS_H
#define TAF_HOOKS_H

#include "modules/hooks/taf-hooks.h"

#include "taf_state.h"

void taf_hooks_init(taf_state_t *state);

void taf_hooks_add_to_queue(taf_hook_t hook);

void taf_hooks_run(lua_State *L, taf_hook_fn fn);

void taf_hooks_deinit();

#endif // TAF_HOOKS_H
