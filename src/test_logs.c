#include "test_logs.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "taf_tui.h"
#include "version.h"

#include "util/files.h"
#include "util/os.h"
#include "util/time.h"

#include <json.h>
#include <string.h>

static test_case_t current = {0};

static FILE *output_log_file;

static char logs_dir[PATH_MAX];
static char output_log_file_path[PATH_MAX];
static char raw_log_file_path[PATH_MAX];

static raw_log_t *raw_log = NULL;
static size_t raw_log_test_ouput_cap;
static int test_index;

static json_object *raw_log_test_output_to_json(raw_log_test_output_t *output) {
    json_object *output_obj = json_object_new_object();
    json_object_object_add(output_obj, "file",
                           json_object_new_string(output->file));
    json_object_object_add(output_obj, "line",
                           json_object_new_int(output->line));
    json_object_object_add(output_obj, "date_time",
                           json_object_new_string(output->date_time));
    json_object_object_add(output_obj, "msg",
                           json_object_new_string(output->msg));
    return output_obj;
}

static json_object *raw_log_test_to_json(raw_log_test_t *test) {
    json_object *test_obj = json_object_new_object();
    json_object_object_add(test_obj, "name",
                           json_object_new_string(test->name));
    json_object_object_add(test_obj, "started",
                           json_object_new_string(test->started));
    json_object_object_add(test_obj, "finished",
                           json_object_new_string(test->finished));
    json_object_object_add(test_obj, "status",
                           json_object_new_string(test->status));
    if (test->failure_reason) {
        json_object_object_add(test_obj, "failure_reason",
                               json_object_new_string(test->failure_reason));
    }
    json_object *tag_arr = json_object_new_array();
    for (size_t i = 0; i < test->tags_count; i++) {
        json_object_array_add(tag_arr, json_object_new_string(test->tags[i]));
    }
    json_object_object_add(test_obj, "tags", tag_arr);
    json_object *output_arr = json_object_new_array();
    for (size_t i = 0; i < test->output_count; i++) {
        json_object_array_add(output_arr,
                              raw_log_test_output_to_json(&test->output[i]));
    }
    json_object_object_add(test_obj, "output", output_arr);
    return test_obj;
}

json_object *taf_raw_log_to_json(raw_log_t *log) {
    json_object *root = json_object_new_object();
    json_object_object_add(root, "taf_version",
                           json_object_new_string(log->taf_version));
    json_object_object_add(root, "os", json_object_new_string(log->os));
    json_object_object_add(root, "os_version",
                           json_object_new_string(log->os_version));
    json_object_object_add(root, "started",
                           json_object_new_string(log->started));
    json_object_object_add(root, "finished",
                           json_object_new_string(log->finished));

    json_object *tag_arr = json_object_new_array();
    for (size_t i = 0; i < log->tags_count; i++) {
        json_object_array_add(tag_arr, json_object_new_string(log->tags[i]));
    }
    json_object_object_add(root, "tags", tag_arr);

    json_object *tests_arr = json_object_new_array();
    for (size_t i = 0; i < log->tests_count; i++) {
        json_object_array_add(tests_arr, raw_log_test_to_json(&log->tests[i]));
    }
    json_object_object_add(root, "tests", tests_arr);

    return root;
}

static inline char *jdup_string(struct json_object *o) {
    return o ? strdup(json_object_get_string(o)) : NULL;
}

static inline size_t jarray_len(struct json_object *arr) {
    return json_object_is_type(arr, json_type_array)
               ? (size_t)json_object_array_length(arr)
               : 0;
}

/* main parser ──────────────────────────────────────────────────────── */
raw_log_t *taf_json_to_raw_log(struct json_object *root) {
    if (!root || !json_object_is_type(root, json_type_object))
        return NULL;

    raw_log_t *log = calloc(1, sizeof *log);
    if (!log)
        return NULL;

    struct json_object *o = NULL;

    if (json_object_object_get_ex(root, "taf_version", &o))
        log->taf_version = jdup_string(o);

    if (json_object_object_get_ex(root, "os", &o))
        log->os = jdup_string(o);

    if (json_object_object_get_ex(root, "os_version", &o))
        log->os_version = jdup_string(o);

    if (json_object_object_get_ex(root, "started", &o))
        log->started = jdup_string(o);

    if (json_object_object_get_ex(root, "finished", &o))
        log->finished = jdup_string(o);

    if (json_object_object_get_ex(root, "tags", &o) &&
        json_object_is_type(o, json_type_array)) {

        log->tags_count = jarray_len(o);
        log->tags = calloc(log->tags_count, sizeof *log->tags);

        for (size_t i = 0; i < log->tags_count; ++i) {
            struct json_object *tag = json_object_array_get_idx(o, (int)i);
            log->tags[i] = jdup_string(tag);
        }
    }

    if (json_object_object_get_ex(root, "tests", &o) &&
        json_object_is_type(o, json_type_array)) {

        log->tests_count = jarray_len(o);
        log->tests = calloc(log->tests_count, sizeof *log->tests);

        for (size_t i = 0; i < log->tests_count; ++i) {
            struct json_object *jt = json_object_array_get_idx(o, (int)i);
            raw_log_test_t *t = &log->tests[i];

            struct json_object *tmp;
            if (json_object_object_get_ex(jt, "name", &tmp))
                t->name = jdup_string(tmp);
            if (json_object_object_get_ex(jt, "started", &tmp))
                t->started = jdup_string(tmp);
            if (json_object_object_get_ex(jt, "finished", &tmp))
                t->finished = jdup_string(tmp);
            if (json_object_object_get_ex(jt, "status", &tmp))
                t->status = jdup_string(tmp);
            if (json_object_object_get_ex(jt, "failure_reason", &tmp))
                t->failure_reason = jdup_string(tmp);

            if (json_object_object_get_ex(jt, "tags", &tmp) &&
                json_object_is_type(tmp, json_type_array)) {

                t->tags_count = jarray_len(tmp);
                t->tags = calloc(t->tags_count, sizeof *t->tags);
                for (size_t k = 0; k < t->tags_count; ++k) {
                    struct json_object *jtag =
                        json_object_array_get_idx(tmp, (int)k);
                    t->tags[k] = jdup_string(jtag);
                }
            }

            if (json_object_object_get_ex(jt, "output", &tmp) &&
                json_object_is_type(tmp, json_type_array)) {

                t->output_count = jarray_len(tmp);
                t->output = calloc(t->output_count, sizeof *t->output);

                for (size_t k = 0; k < t->output_count; ++k) {
                    struct json_object *jo =
                        json_object_array_get_idx(tmp, (int)k);
                    raw_log_test_output_t *out = &t->output[k];

                    struct json_object *jfield;
                    if (json_object_object_get_ex(jo, "file", &jfield))
                        out->file = jdup_string(jfield);
                    if (json_object_object_get_ex(jo, "date_time", &jfield))
                        out->date_time = jdup_string(jfield);
                    if (json_object_object_get_ex(jo, "msg", &jfield))
                        out->msg = jdup_string(jfield);
                    if (json_object_object_get_ex(jo, "line", &jfield))
                        out->line = json_object_get_int(jfield);
                }
            }
        }
    }

    return log;
}

void taf_log_tests_create(int amount) {

    taf_tui_set_test_amount(amount);

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    project_parsed_t *proj = get_parsed_project();
    cmd_test_options *opts = cmd_parser_get_test_options();

    if (proj->multitarget) {
        snprintf(logs_dir, PATH_MAX, "%s/logs/%s", proj->project_path,
                 opts->target);
    } else {
        snprintf(logs_dir, PATH_MAX, "%s/logs", proj->project_path);
    }
    if (!directory_exists(logs_dir)) {
        create_directory(logs_dir, MKDIR_MODE);
    }

    snprintf(raw_log_file_path, PATH_MAX, "%s/test_run_%s_raw.json", logs_dir,
             time_str);

    snprintf(output_log_file_path, PATH_MAX, "%s/test_run_%s_output.log",
             logs_dir, time_str);
    output_log_file = fopen(output_log_file_path, "w");

    raw_log = calloc(1, sizeof *raw_log);
    raw_log->tests = calloc(amount, sizeof(raw_log_test_t));
    raw_log->tests_count = amount;
    raw_log->tags_count = opts->tags_amount;
    raw_log->tags = opts->tags;
    raw_log->started = strdup(time_str);
    raw_log->taf_version = TAF_VERSION;
#if defined(__APPLE__)
    raw_log->os = "macos";
#elif defined(__linux__)
    raw_log->os = "linux";
#elif defined(_WIN32) || defined(_WIN64)
    raw_log->os = "windows";
#else
    raw_log->os = "unknown";
#endif
    raw_log->os_version = get_os_string();
}

void taf_log_test(const char *file, int line, const char *buffer) {
    taf_tui_log(file, line, buffer);

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    fprintf(output_log_file, "[%s][%s][%s:%d]: %s\n\n", time_str, current.name,
            file, line, buffer);

    raw_log_test_t *test = &raw_log->tests[test_index];
    if (test->output_count >= raw_log_test_ouput_cap) {
        raw_log_test_ouput_cap *= 2;
        test->output = realloc(test->output, sizeof(raw_log_test_output_t) *
                                                 raw_log_test_ouput_cap);
    }
    test->output[test->output_count].file = strdup(file);
    test->output[test->output_count].date_time = strdup(time_str);
    test->output[test->output_count].msg = strdup(buffer);
    test->output[test->output_count].line = line;
    test->output_count++;
}

void taf_log_test_started(int index, test_case_t test_case) {
    test_index = index - 1;
    current = test_case;
    taf_tui_set_current_test(index, test_case.name);

    char time_str[TS_LEN];
    get_date_time_now(time_str);
    fprintf(output_log_file, "[%s][%s]: Test %d Started.\n\n", time_str,
            test_case.name, index);

    raw_log_test_t *test = &raw_log->tests[index - 1];
    test->started = strdup(time_str);

    test->tags = malloc(sizeof(char *) * test_case.tags.amount);
    for (size_t i = 0; i < test_case.tags.amount; i++) {
        test->tags[i] = strdup(test_case.tags.tags[i]);
    }
    test->tags_count = test_case.tags.amount;

    test->name = strdup(test_case.name);
    raw_log_test_ouput_cap = 2;
    test->output =
        malloc(sizeof(raw_log_test_output_t) * raw_log_test_ouput_cap);
    test->output_count = 0;
    test->failure_reason = NULL;
}

void taf_log_test_passed(int index, test_case_t test_case) {

    taf_tui_test_passed(index, test_case.name);

    char time_str[TS_LEN];
    get_date_time_now(time_str);
    fprintf(output_log_file, "[%s][%s]: Test %d Passed.\n\n", time_str,
            test_case.name, index);

    raw_log_test_t *test = &raw_log->tests[index - 1];
    test->finished = strdup(time_str);
    test->status = "passed";
}

void taf_log_test_failed(int index, test_case_t test_case, const char *msg) {

    taf_tui_test_failed(index, test_case.name, msg);

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    fprintf(output_log_file, "[%s][%s]: Test %d Failed: %s\n\n", time_str,
            test_case.name, index, msg);

    raw_log_test_t *test = &raw_log->tests[index - 1];
    test->finished = strdup(time_str);
    test->status = "failed";
    test->failure_reason = strdup(msg);
}

void taf_raw_log_free(raw_log_t *log) {
    if (!log)
        return;

    free(log->taf_version);
    free(log->os_version);
    free(log->started);
    free(log->finished);

    for (size_t i = 0; i < log->tags_count; ++i)
        free(log->tags[i]);
    free(log->tags);

    for (size_t i = 0; i < log->tests_count; ++i) {
        raw_log_test_t *t = &log->tests[i];

        free(t->name);
        free(t->started);
        free(t->finished);
        free(t->status);
        free(t->failure_reason);

        for (size_t k = 0; k < t->tags_count; ++k)
            free(t->tags[k]);
        free(t->tags);

        for (size_t k = 0; k < t->output_count; ++k) {
            raw_log_test_output_t *o = &t->output[k];
            free(o->file);
            free(o->date_time);
            free(o->msg);
        }
        free(t->output);
    }
    free(log->tests);

    free(log);
}

void taf_log_tests_finalize() {

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    raw_log->finished = strdup(time_str);

    json_object *raw_log_root = taf_raw_log_to_json(raw_log);

    if (json_object_to_file_ext(raw_log_file_path, raw_log_root,
                                JSON_C_TO_STRING_SPACED |
                                    JSON_C_TO_STRING_PRETTY |
                                    JSON_C_TO_STRING_NOSLASHESCAPE) == -1) {
        // TODO: internal log: Error saving
    }

    // Cleanup
    free(raw_log->finished);
    free(raw_log->os_version);
    free(raw_log->started);
    for (size_t i = 0; i < raw_log->tags_count; i++) {
        free(raw_log->tags[i]);
    }
    free(raw_log->tags);
    raw_log->tags_count = 0;

    for (size_t i = 0; i < raw_log->tests_count; i++) {
        raw_log_test_t *test = &raw_log->tests[i];
        free(test->started);
        free(test->finished);
        free(test->failure_reason);
        free(test->name);
        for (size_t j = 0; j < test->tags_count; j++) {
            free(test->tags[j]);
        }
        free(test->tags);
        for (size_t j = 0; j < test->output_count; j++) {
            free(test->output[j].msg);
            free(test->output[j].date_time);
            free(test->output[j].file);
        }
        free(test->output);
    }
    free(raw_log->tests);
    free(raw_log);

    json_object_put(raw_log_root);

    fflush(output_log_file);
    fclose(output_log_file);

    // Create 'latest' symlinks:
    char latest_log[PATH_MAX];
    snprintf(latest_log, PATH_MAX, "%s/test_run_latest_output.log", logs_dir);
    char latest_raw[PATH_MAX];
    snprintf(latest_raw, PATH_MAX, "%s/test_run_latest_raw.json", logs_dir);

    replace_symlink(output_log_file_path, latest_log);
    replace_symlink(raw_log_file_path, latest_raw);
}
