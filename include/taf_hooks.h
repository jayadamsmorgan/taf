#ifndef TAF_HOOKS_H
#define TAF_HOOKS_H

#include "modules/hooks/taf-hooks.h"

void taf_hooks_init();

void taf_hooks_add_to_queue(taf_hook_t hook);

void taf_hooks_run(lua_State *L, taf_hook_fn fn);

void taf_hooks_deinit();

#endif // TAF_HOOKS_H
