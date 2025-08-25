#include "taf_hooks.h"

#include "internal_logging.h"

#include "test_logs.h"

#include <stdlib.h>

static da_t *hooks_test_run_started;
static da_t *hooks_test_started;
static da_t *hooks_test_finished;
static da_t *hooks_test_run_finished;

static da_t *hook_started_cbs;
static da_t *hook_finished_cbs;
static da_t *hook_failed_cbs;

static taf_state_t *taf_state = NULL;

void taf_hooks_register_hook_started_cb(hook_cb cb) {
    da_append(hook_started_cbs, (void *)&cb);
}

void taf_hooks_register_hook_finished_cb(hook_cb cb) {
    da_append(hook_finished_cbs, (void *)&cb);
}

void taf_hooks_register_hook_failed_cb(hook_err_cb cb) {
    da_append(hook_failed_cbs, (void *)&cb);
}

static inline da_t *taf_get_hooks(taf_hook_fn fn) {
    switch (fn) {
    case TAF_HOOK_FN_TEST_RUN_STARTED:
        return hooks_test_run_started;
    case TAF_HOOK_FN_TEST_STARTED:
        return hooks_test_started;
    case TAF_HOOK_FN_TEST_FINISHED:
        return hooks_test_finished;
    case TAF_HOOK_FN_TEST_RUN_FINISHED:
        return hooks_test_run_finished;
    }
    return NULL;
}

void taf_hooks_init(taf_state_t *state) {
    LOG("Initializing TAF hooks...");

    taf_state = state;

    hooks_test_run_started = da_init(1, sizeof(taf_hook_t));
    hooks_test_started = da_init(1, sizeof(taf_hook_t));
    hooks_test_finished = da_init(1, sizeof(taf_hook_t));
    hooks_test_run_finished = da_init(1, sizeof(taf_hook_t));

    hook_started_cbs = da_init(1, sizeof(hook_cb));
    hook_finished_cbs = da_init(1, sizeof(hook_cb));
    hook_failed_cbs = da_init(1, sizeof(hook_err_cb));

    LOG("Successfully initialized TAF hooks.");
}

void taf_hooks_add_to_queue(taf_hook_t hook) {
    LOG("Adding hook with type %d and ref %d", hook.fn, hook.ref);
    da_t *hooks_da = taf_get_hooks(hook.fn);
    da_append(hooks_da, &hook);
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
    size_t tests_count = da_size(taf_state->tests);
    if (tests_count != 0) {
        taf_state_test_t *t = da_get(taf_state->tests, tests_count - 1);

        lua_newtable(L); // [ context, test ]
        push_string(L, "name", t->name);
        push_string(L, "started", t->started);
        if (t->finished)
            push_string(L, "finished", t->finished);
        if (t->status)
            push_string(L, "status", t->status_str);

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

static void run_taf_hook_started_cbs(taf_hook_fn fn) {
    size_t size = da_size(hook_started_cbs);
    for (size_t i = 0; i < size; ++i) {
        hook_cb *cb = da_get(hook_started_cbs, i);
        if (cb && *cb) {
            (*cb)(fn);
        }
    }
}

static void run_taf_hook_finished_cbs(taf_hook_fn fn) {
    size_t size = da_size(hook_finished_cbs);
    for (size_t i = 0; i < size; ++i) {
        hook_cb *cb = da_get(hook_finished_cbs, i);
        if (cb && *cb) {
            (*cb)(fn);
        }
    }
}

static void run_taf_hook_failed_cbs(taf_hook_fn fn, const char *msg) {
    size_t size = da_size(hook_failed_cbs);
    for (size_t i = 0; i < size; ++i) {
        hook_err_cb *cb = da_get(hook_failed_cbs, i);
        if (cb && *cb) {
            (*cb)(fn, msg);
        }
    }
}

void taf_hooks_run(lua_State *L, taf_hook_fn fn) {
    LOG("Running all hooks with type %d...", fn);
    da_t *hooks = taf_get_hooks(fn);
    size_t hooks_count = da_size(hooks);
    if (hooks_count == 0) {
        LOG("No hooks found for type %d", fn);
        return;
    }
    for (size_t i = 0; i < hooks_count; i++) {
        run_taf_hook_started_cbs(fn);
        taf_hook_t *hook = da_get(hooks, i);
        int ref = hook->ref;
        LOG("Running hook with type %d and ref %d", fn, ref);
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        hooks_context_push(L);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            const char *err = lua_tostring(L, -1);
            LOG("Error running hook with type %d and ref %d:\n%s", fn, ref,
                err);
            run_taf_hook_failed_cbs(fn, err);
            continue;
        }
        LOG("Successfully ran hook with type %d and ref %d", fn, ref);
        run_taf_hook_finished_cbs(fn);
    }
    LOG("Successfully ran all hooks with type %d.", fn);
}

static void free_hooks_da(da_t *hooks, lua_State *L) {
    size_t size = da_size(hooks);
    for (size_t i = 0; i < size; ++i) {
        taf_hook_t *hook = da_get(hooks, i);
        luaL_unref(L, LUA_REGISTRYINDEX, hook->ref);
    }
}

void taf_hooks_deinit(lua_State *L) {
    LOG("Deinitializing TAF hooks...");

    free_hooks_da(hooks_test_run_started, L);
    free_hooks_da(hooks_test_run_started, L);
    free_hooks_da(hooks_test_started, L);
    free_hooks_da(hooks_test_finished, L);
    free_hooks_da(hooks_test_run_finished, L);

    da_free(hook_started_cbs);
    da_free(hook_finished_cbs);
    da_free(hook_failed_cbs);

    LOG("Successfully deinitialized TAF hooks.");
}
