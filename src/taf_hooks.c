#include "taf_hooks.h"

#include "project_parser.h"
#include "taf_tui.h"

#include "cmd_parser.h"
#include "headless.h"
#include "internal_logging.h"

#include "test_logs.h"
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

static inline void push_string(lua_State *L, const char *key,
                               const char *value) {
    lua_pushstring(L, value);
    lua_setfield(L, -2, key);
}

static inline void push_output(lua_State *L, const taf_state_test_output_t *o) {
    lua_newtable(L);

    push_string(L, "msg", o->msg);
    push_string(L, "level", taf_log_level_to_str(o->level));
    push_string(L, "date_time", o->date_time);
    push_string(L, "file", o->file);

    lua_pushinteger(L, (lua_Integer)o->line);
    lua_setfield(L, -2, "line");
}

static int hooks_context_push(lua_State *L) {
    lua_newtable(L);

    taf_state_t *taf_state = taf_log_get_state();

    // context.test_run
    lua_newtable(L); // [ context, test_run ]
    push_string(L, "project_name", taf_state->project_name);
    push_string(L, "taf_version", taf_state->taf_version);
    push_string(L, "os", taf_state->os);
    push_string(L, "os_version", taf_state->os_version);
    if (taf_state->target)
        push_string(L, "target", taf_state->target);
    push_string(L, "started", taf_state->started);
    if (taf_state->finished)
        push_string(L, "finished", taf_state->finished);

    // test_run.tags (array)
    lua_newtable(L); // [ context, test_run, tags ]
    size_t tags_count = da_size(taf_state->tags);
    for (size_t i = 0; i < tags_count; i++) {
        char **tag = da_get(taf_state->tags, i);
        lua_pushstring(L, *tag);               // [ ..., tags, str ]
        lua_seti(L, -2, (lua_Integer)(i + 1)); // tags[i+1] = str
    }
    lua_setfield(L, -2, "tags"); // test_run.tags = tags

    lua_setfield(L, -2, "test_run"); // context.test_run = test_run

    // context.test
    int test_index = taf_log_get_test_index();
    if (test_index != -1) {
        taf_state_test_t *t = &taf_state->tests[test_index];

        lua_newtable(L); // [ context, test ]
        push_string(L, "name", t->name);
        push_string(L, "started", t->started);
        if (t->finished)
            push_string(L, "finished", t->finished);
        if (t->status)
            push_string(L, "status", t->status);

        // test.tags (array)
        lua_newtable(L); // [ context, test, tags ]
        size_t tags_count = da_size(t->tags);
        for (size_t i = 0; i < tags_count; i++) {
            char **tag = da_get(t->tags, i);
            lua_pushstring(L, *tag);
            lua_seti(L, -2, (lua_Integer)(i + 1));
        }
        lua_setfield(L, -2, "tags"); // test.tags = tags

        // test.outputs (array of objects)
        lua_newtable(L); // [ context, test, outputs ]
        size_t outputs_count = da_size(t->outputs);
        for (size_t i = 0; i < outputs_count; i++) {
            taf_state_test_output_t *o = da_get(t->outputs, i);
            push_output(L, o);                     // pushes elem
            lua_seti(L, -2, (lua_Integer)(i + 1)); // outputs[i+1] = elem
        }
        lua_setfield(L, -2, "outputs");

        // test.failure_reasons (array of objects)
        lua_newtable(L); // [ context, test, failure_reasons ]
        size_t failure_reasons_count = da_size(t->failure_reasons);
        for (size_t i = 0; i < failure_reasons_count; i++) {
            taf_state_test_output_t *o = da_get(t->failure_reasons, i);
            push_output(L, o);
            lua_seti(L, -2, (lua_Integer)(i + 1));
        }
        lua_setfield(L, -2, "failure_reasons");

        if (t->teardown_start)
            push_string(L, "teardown_start", t->teardown_start);

        // test.teardown_outputs (array of objects)
        lua_newtable(L); // [ context, test, teardown_outputs ]
        size_t teardown_outputs_count = da_size(t->teardown_outputs);
        for (size_t i = 0; i < teardown_outputs_count; i++) {
            taf_state_test_output_t *o = da_get(t->teardown_outputs, i);
            push_output(L, o);
            lua_seti(L, -2, (lua_Integer)(i + 1));
        }
        lua_setfield(L, -2, "teardown_outputs");

        // test.teardown_errors (array of objects)
        lua_newtable(L); // [ context, test, teardown_errors ]
        size_t teardown_errors_count = da_size(t->teardown_errors);
        for (size_t i = 0; i < teardown_errors_count; i++) {
            taf_state_test_output_t *o = da_get(t->teardown_errors, i);
            push_output(L, o);
            lua_seti(L, -2, (lua_Integer)(i + 1));
        }
        lua_setfield(L, -2, "teardown_errors");

        lua_setfield(L, -2, "test"); // context.test = test
    }

    // context.logs
    lua_newtable(L); // [ context, logs ]

    char *logs_dir = taf_log_get_logs_dir();
    if (logs_dir) {
        push_string(L, "dir", logs_dir);
    }
    char *raw_log_file_path = taf_log_get_raw_log_file_path();
    if (raw_log_file_path) {
        push_string(L, "raw_log_path", raw_log_file_path);
    }
    char *output_log_file_path = taf_log_get_output_log_file_path();
    if (output_log_file_path) {
        push_string(L, "output_log_path", output_log_file_path);
    }
    lua_setfield(L, -2, "logs"); // context.logs = logs

    return 1; // context remains on the stack
}

void taf_hooks_run(lua_State *L, taf_hook_fn fn) {
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
        hooks_context_push(L);
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
