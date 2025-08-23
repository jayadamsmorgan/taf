#include "taf_state.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "taf_vars.h"
#include "test_case.h"
#include "util/time.h"
#include "version.h"

#include "util/os.h"

#include <string.h>

static inline char *jdup_string(struct json_object *o) {
    return o ? strdup(json_object_get_string(o)) : NULL;
}

static inline void add_string_if(json_object *obj, const char *key,
                                 const char *val) {
    if (val)
        json_object_object_add(obj, key, json_object_new_string(val));
}

static inline void add_string_len_if(json_object *obj, const char *key,
                                     const char *val, size_t len) {
    if (val)
        json_object_object_add(obj, key,
                               json_object_new_string_len(val, (int)len));
}

#define JGET_STR_DUP(OBJ, KEY, DEST)                                           \
    do {                                                                       \
        struct json_object *tmp__;                                             \
        if (json_object_object_get_ex((OBJ), (KEY), &tmp__)) {                 \
            (DEST) = jdup_string(tmp__);                                       \
        }                                                                      \
    } while (0)

#define JGET_INT(OBJ, KEY, DEST)                                               \
    do {                                                                       \
        struct json_object *tmp__;                                             \
        if (json_object_object_get_ex((OBJ), (KEY), &tmp__)) {                 \
            (DEST) = json_object_get_int(tmp__);                               \
        }                                                                      \
    } while (0)

static json_object *
taf_state_test_output_to_json(const taf_state_test_output_t *output) {
    json_object *o = json_object_new_object();
    add_string_if(o, "file", output->file);
    json_object_object_add(o, "line", json_object_new_int(output->line));
    add_string_if(o, "date_time", output->date_time);
    add_string_if(o, "level", taf_log_level_to_str(output->level));
    add_string_len_if(o, "msg", output->msg, output->msg_len);
    return o;
}

static void taf_state_test_output_from_json(json_object *jo,
                                            taf_state_test_output_t *out) {
    memset(out, 0, sizeof *out);
    JGET_STR_DUP(jo, "file", out->file);
    JGET_STR_DUP(jo, "date_time", out->date_time);
    JGET_STR_DUP(jo, "msg", out->msg);
    if (out->msg)
        out->msg_len = strlen(out->msg);

    struct json_object *lvl;
    if (json_object_object_get_ex(jo, "level", &lvl))
        out->level = taf_log_level_from_str(json_object_get_string(lvl));

    JGET_INT(jo, "line", out->line);
}

static json_object *da_strings_to_json_array(const da_t *arr) {
    json_object *a = json_object_new_array();
    size_t n = da_size((da_t *)arr);
    for (size_t i = 0; i < n; ++i) {
        const char *s = *(char *const *)da_get((da_t *)arr, i);
        json_object_array_add(a, json_object_new_string(s ? s : ""));
    }
    return a;
}

static da_t *json_array_to_da_strings(json_object *a) {
    if (!a || !json_object_is_type(a, json_type_array))
        return NULL;
    size_t n = (size_t)json_object_array_length(a);
    da_t *out = da_init(n, sizeof(char *));
    for (size_t i = 0; i < n; ++i) {
        json_object *ji = json_object_array_get_idx(a, (int)i);
        const char *s = json_object_get_string(ji);
        char *dup = s ? strdup(s) : NULL;
        if (!da_append(out, &dup)) {
            da_free(out);
            return NULL;
        }
    }
    return out;
}

static da_t *json_object_to_vars(json_object *v) {
    if (!v || !json_object_is_type(v, json_type_object))
        return NULL;

    da_t *out = da_init(1, sizeof(taf_var_entry_t));
    json_object_iter i;
    json_object_object_foreachC(v, i) {
        taf_var_entry_t e = {0};
        e.name = strdup(i.key);
        if (json_object_is_type(i.val, json_type_string)) {
            e.final_value = strdup(json_object_get_string(i.val));
        }
        da_append(out, &e);
    }

    return out;
}

static json_object *da_outputs_to_json_array(const da_t *arr) {
    json_object *a = json_object_new_array();
    size_t n = da_size((da_t *)arr);
    for (size_t i = 0; i < n; ++i) {
        const taf_state_test_output_t *o =
            (const taf_state_test_output_t *)da_get((da_t *)arr, i);
        json_object_array_add(a, taf_state_test_output_to_json(o));
    }
    return a;
}

static da_t *json_array_to_da_outputs(json_object *a) {
    if (!a || !json_object_is_type(a, json_type_array))
        return NULL;
    size_t n = (size_t)json_object_array_length(a);
    da_t *out = da_init(n, sizeof(taf_state_test_output_t));
    for (size_t i = 0; i < n; ++i) {
        json_object *ji = json_object_array_get_idx(a, (int)i);
        taf_state_test_output_t item;
        taf_state_test_output_from_json(ji, &item);
        if (!da_append(out, &item)) {
            da_free(out);
            return NULL;
        }
    }
    return out;
}

static json_object *taf_state_test_to_json(const taf_state_test_t *t) {
    json_object *o = json_object_new_object();

    add_string_if(o, "name", t->name);
    add_string_if(o, "description", t->description);
    add_string_if(o, "started", t->started);
    add_string_if(o, "finished", t->finished);
    add_string_if(o, "teardown_start", t->teardown_start);
    add_string_if(o, "status", t->status);

    /* tags */
    json_object_object_add(o, "tags", da_strings_to_json_array(t->tags));

    /* failure_reasons / output / teardown_output / teardown_errors */
    if (t->failure_reasons)
        json_object_object_add(o, "failure_reasons",
                               da_outputs_to_json_array(t->failure_reasons));
    else
        json_object_object_add(o, "failure_reasons", json_object_new_array());

    if (t->outputs)
        json_object_object_add(o, "output",
                               da_outputs_to_json_array(t->outputs));
    else
        json_object_object_add(o, "output", json_object_new_array());

    if (t->teardown_outputs)
        json_object_object_add(o, "teardown_output",
                               da_outputs_to_json_array(t->teardown_outputs));
    else
        json_object_object_add(o, "teardown_output", json_object_new_array());

    if (t->teardown_errors)
        json_object_object_add(o, "teardown_errors",
                               da_outputs_to_json_array(t->teardown_errors));
    else
        json_object_object_add(o, "teardown_errors", json_object_new_array());

    return o;
}

static json_object *taf_state_vars_to_json(da_t *vars) {
    json_object *o = json_object_new_object();

    size_t vars_size = da_size(vars);
    for (size_t i = 0; i < vars_size; ++i) {
        taf_var_entry_t *var = da_get(vars, i);
        json_object_object_add(o, var->name,
                               json_object_new_string(var->final_value));
    }

    return o;
}

static void taf_state_test_from_json(json_object *jt, taf_state_test_t *t) {
    memset(t, 0, sizeof *t);

    JGET_STR_DUP(jt, "name", t->name);
    JGET_STR_DUP(jt, "description", t->description);
    JGET_STR_DUP(jt, "started", t->started);
    JGET_STR_DUP(jt, "finished", t->finished);
    JGET_STR_DUP(jt, "teardown_start", t->teardown_start);
    JGET_STR_DUP(jt, "status", t->status);

    json_object *tmp;

    if (json_object_object_get_ex(jt, "tags", &tmp))
        t->tags = json_array_to_da_strings(tmp);

    if (json_object_object_get_ex(jt, "failure_reasons", &tmp))
        t->failure_reasons = json_array_to_da_outputs(tmp);

    if (json_object_object_get_ex(jt, "output", &tmp))
        t->outputs = json_array_to_da_outputs(tmp);

    if (json_object_object_get_ex(jt, "teardown_output", &tmp))
        t->teardown_outputs = json_array_to_da_outputs(tmp);

    if (json_object_object_get_ex(jt, "teardown_errors", &tmp))
        t->teardown_errors = json_array_to_da_outputs(tmp);
}

/* ----- state <-> json (public API) ------------------------------------- */

json_object *taf_state_to_json(taf_state_t *log) {
    if (!log)
        return NULL;

    json_object *root = json_object_new_object();

    add_string_if(root, "project_name", log->project_name);
    add_string_if(root, "taf_version", log->taf_version);
    add_string_if(root, "os", log->os);
    add_string_if(root, "os_version", log->os_version);
    add_string_if(root, "started", log->started);
    add_string_if(root, "finished", log->finished);
    add_string_if(root, "target", log->target);

    json_object_object_add(root, "variables",
                           taf_state_vars_to_json(log->vars));

    json_object_object_add(root, "tags",
                           log->tags ? da_strings_to_json_array(log->tags)
                                     : json_object_new_array());

    json_object *tests_arr = json_object_new_array();
    for (size_t i = 0; i < log->total_amount; ++i) {
        json_object_array_add(tests_arr,
                              taf_state_test_to_json(&log->tests[i]));
    }
    json_object_object_add(root, "tests", tests_arr);

    json_object_object_add(root, "total_amount",
                           json_object_new_int((int)log->total_amount));
    json_object_object_add(root, "passed_amount",
                           json_object_new_int((int)log->passed_amount));
    json_object_object_add(root, "failed_amount",
                           json_object_new_int((int)log->failed_amount));
    json_object_object_add(root, "finished_amount",
                           json_object_new_int((int)log->finished_amount));

    return root;
}

taf_state_t *taf_state_from_json(json_object *root) {
    if (!root || !json_object_is_type(root, json_type_object))
        return NULL;

    taf_state_t *log = (taf_state_t *)calloc(1, sizeof *log);
    if (!log)
        return NULL;

    JGET_STR_DUP(root, "project_name", log->project_name);
    JGET_STR_DUP(root, "taf_version", log->taf_version);
    JGET_STR_DUP(root, "os", log->os);
    JGET_STR_DUP(root, "os_version", log->os_version);
    JGET_STR_DUP(root, "started", log->started);
    JGET_STR_DUP(root, "finished", log->finished);
    JGET_STR_DUP(root, "target", log->target);

    json_object *tmp;

    if (json_object_object_get_ex(root, "variables", &tmp))
        log->vars = json_object_to_vars(tmp);

    if (json_object_object_get_ex(root, "tags", &tmp))
        log->tags = json_array_to_da_strings(tmp);

    if (json_object_object_get_ex(root, "tests", &tmp) &&
        json_object_is_type(tmp, json_type_array)) {

        size_t n = (size_t)json_object_array_length(tmp);
        log->total_amount = n;
        log->tests = (taf_state_test_t *)calloc(n, sizeof *log->tests);

        for (size_t i = 0; i < n; ++i) {
            json_object *jt = json_object_array_get_idx(tmp, (int)i);
            taf_state_test_from_json(jt, &log->tests[i]);
        }
    }

    JGET_INT(root, "passed_amount", log->passed_amount);
    JGET_INT(root, "failed_amount", log->failed_amount);
    JGET_INT(root, "finished_amount", log->finished_amount);

    return log;
}

taf_state_t *taf_state_new() {
    cmd_test_options *opts = cmd_parser_get_test_options();
    project_parsed_t *proj = get_parsed_project();

    size_t amount;
    test_case_get_all(&amount);

    char time[TS_LEN];
    get_date_time_now(time);

    taf_state_t *taf_state = calloc(1, sizeof *taf_state);
    taf_state->project_name = strdup(proj->project_name);
    taf_state->tests = calloc(amount, sizeof(taf_state_test_t));
    taf_state->total_amount = amount;
    size_t tags_size = da_size(opts->tags);
    taf_state->tags = da_init(tags_size, sizeof(char *));
    for (size_t i = 0; i < tags_size; i++) {
        char **tag = da_get(opts->tags, i);
        char *to_cpy = strdup(*tag);
        da_append(taf_state->tags, &to_cpy);
    }
    da_t *vars = taf_get_vars();
    if (vars) {
        size_t vars_size = da_size(vars);
        taf_state->vars = da_init(vars_size, sizeof(taf_var_entry_t));
        for (size_t i = 0; i < vars_size; ++i) {
            taf_var_entry_t *e = da_get(vars, i);
            size_t values_count = da_size(e->values);
            da_t *values_cpy = da_init(values_count, sizeof(char *));
            for (size_t j = 0; j < values_count; ++j) {
                char **value = da_get(e->values, j);
                char *tmp = strdup(*value);
                da_append(values_cpy, &tmp);
            }
            taf_var_entry_t cpy = {
                .name = strdup(e->name),
                .values = values_cpy,
                .is_scalar = e->is_scalar,
                .scalar = e->scalar ? strdup(e->scalar) : NULL,
                .def_value = e->def_value ? strdup(e->def_value) : NULL,
                .final_value = strdup(e->final_value),
            };
            da_append(taf_state->vars, &cpy);
        }
    }
    taf_state->started = strdup(time);
    taf_state->taf_version = strdup(TAF_VERSION);
    if (opts->target) {
        taf_state->target = strdup(opts->target);
    } else {
        taf_state->target = NULL;
    }
#if defined(__APPLE__)
    taf_state->os = strdup("macos");
#elif defined(__linux__)
    taf_state->os = strdup("linux");
#elif defined(_WIN32) || defined(_WIN64)
    taf_state->os = strdup("windows");
#else
    taf_state->os = strdup("unknown");
#endif
    taf_state->os_version = get_os_string();

    return taf_state;
}

static void taf_state_test_output_free(taf_state_test_output_t *o) {
    if (!o)
        return;
    free(o->file);
    free(o->date_time);
    free(o->msg);
    o->file = o->date_time = o->msg = NULL;
}

static void da_free_vars(da_t *vars) {
    if (!vars)
        return;
    size_t n = da_size(vars);
    for (size_t i = 0; i < n; ++i) {
        taf_var_entry_t *var = da_get(vars, i);
        if (var) {
            free(var->name);
            free(var->def_value);
            free(var->final_value);
            free(var->scalar);
            size_t values_size = da_size(var->values);
            for (size_t j = 0; j < values_size; j++) {
                char **value = da_get(var->values, j);
                free(*value);
            }
            da_free(var->values);
        }
    }
    da_free(vars);
}

static void da_free_strings(da_t *arr) {
    if (!arr)
        return;
    size_t n = da_size(arr);
    for (size_t i = 0; i < n; ++i) {
        char **p = da_get(arr, i);
        if (p && *p) {
            free(*p);
        }
    }
    da_free(arr);
}

static void da_free_outputs(da_t *arr) {
    if (!arr)
        return;
    size_t n = da_size(arr);
    for (size_t i = 0; i < n; ++i) {
        taf_state_test_output_t *o = da_get(arr, i);
        taf_state_test_output_free(o);
    }
    da_free(arr);
}

static void taf_state_test_free(taf_state_test_t *t) {
    if (!t)
        return;

    free(t->name);
    free(t->description);
    free(t->started);
    free(t->finished);
    free(t->teardown_start);
    free(t->status);

    da_free_strings(t->tags);
    da_free_outputs(t->failure_reasons);
    da_free_outputs(t->outputs);
    da_free_outputs(t->teardown_outputs);
    da_free_outputs(t->teardown_errors);
}

void taf_state_free(taf_state_t *state) {
    if (!state)
        return;

    free(state->project_name);
    free(state->taf_version);
    free(state->os);
    free(state->os_version);
    free(state->started);
    free(state->finished);
    free(state->target);

    da_free_vars(state->vars);
    da_free_strings(state->tags);

    if (state->tests) {
        for (size_t i = 0; i < state->total_amount; ++i) {
            taf_state_test_free(&state->tests[i]);
        }
        free(state->tests);
    }

    free(state);
}
