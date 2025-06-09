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

static bool no_logs = false;
static taf_log_level log_level;

static FILE *output_log_file;

static char logs_dir[PATH_MAX];
static char output_log_file_path[PATH_MAX];
static char raw_log_file_path[PATH_MAX];

static raw_log_t *raw_log = NULL;
static size_t raw_log_test_output_cap;
static size_t raw_log_test_failure_cap;
static int test_index;

static json_object *raw_log_test_output_to_json(raw_log_test_output_t *output) {
    json_object *output_obj = json_object_new_object();
    json_object_object_add(output_obj, "file",
                           json_object_new_string(output->file));
    json_object_object_add(output_obj, "line",
                           json_object_new_int(output->line));
    json_object_object_add(output_obj, "date_time",
                           json_object_new_string(output->date_time));
    json_object_object_add(
        output_obj, "level",
        json_object_new_string(taf_log_level_to_str(output->level)));
    json_object_object_add(
        output_obj, "msg",
        json_object_new_string_len(output->msg, output->msg_len));
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
    if (test->failure_reasons_count != 0) {
        json_object *fail_reasons_arr = json_object_new_array();
        for (size_t i = 0; i < test->failure_reasons_count; i++) {
            json_object_array_add(
                fail_reasons_arr,
                raw_log_test_output_to_json(&test->failure_reasons[i]));
        }
        json_object_object_add(test_obj, "failure_reasons", fail_reasons_arr);
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
            if (json_object_object_get_ex(jt, "failure_reasons", &tmp) &&
                json_object_is_type(tmp, json_type_array)) {

                t->failure_reasons_count = jarray_len(tmp);
                t->failure_reasons = calloc(t->failure_reasons_count,
                                            sizeof *t->failure_reasons);
                for (size_t k = 0; k < t->failure_reasons_count; ++k) {
                    struct json_object *jo =
                        json_object_array_get_idx(tmp, (int)k);
                    raw_log_test_output_t *out = &t->failure_reasons[k];

                    struct json_object *jfield;
                    if (json_object_object_get_ex(jo, "file", &jfield))
                        out->file = jdup_string(jfield);
                    if (json_object_object_get_ex(jo, "date_time", &jfield))
                        out->date_time = jdup_string(jfield);
                    if (json_object_object_get_ex(jo, "msg", &jfield))
                        out->msg = jdup_string(jfield);
                    if (json_object_object_get_ex(jo, "level", &jfield))
                        out->level = taf_log_level_from_str(
                            json_object_get_string(jfield));
                    if (json_object_object_get_ex(jo, "line", &jfield))
                        out->line = json_object_get_int(jfield);
                }
            }

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
                    if (json_object_object_get_ex(jo, "level", &jfield))
                        out->level = taf_log_level_from_str(
                            json_object_get_string(jfield));
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

    cmd_test_options *opts = cmd_parser_get_test_options();
    if (opts->no_logs) {
        no_logs = true;
        return;
    }

    log_level = opts->log_level;

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    project_parsed_t *proj = get_parsed_project();

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

static char *memdup0(const void *src, size_t len) {
    char *p = malloc(len + 1);
    if (!p)
        return NULL;
    memcpy(p, src, len);
    p[len] = '\0';
    return p;
}

void taf_log_test(taf_log_level level, const char *file, int line,
                  const char *buffer, size_t buffer_len) {

    taf_tui_log(level, file, line, buffer);

    if (!no_logs && level <= log_level) {
        char ts[TS_LEN];
        get_date_time_now(ts);

        fprintf(output_log_file, "[%s][%s][%s][%s:%d]: ", ts,
                taf_log_level_to_str(level), current.name, file, line);

        fwrite(buffer, 1, buffer_len, output_log_file);
        fputc('\n', output_log_file);
        fputc('\n', output_log_file);
    }

    raw_log_test_t *t = &raw_log->tests[test_index];

    if (t->output_count >= raw_log_test_output_cap) {
        raw_log_test_output_cap *= 2;
        t->output =
            realloc(t->output, raw_log_test_output_cap * sizeof *t->output);
    }

    raw_log_test_output_t *out = &t->output[t->output_count++];
    out->level = level;
    out->msg = memdup0(buffer, buffer_len);
    out->msg_len = buffer_len;
    out->file = strdup(file);
    out->line = line;
    char ts2[TS_LEN];
    get_date_time_now(ts2);
    out->date_time = strdup(ts2);

    if (level == TAF_LOG_LEVEL_ERROR) {
        if (t->failure_reasons_count >= raw_log_test_failure_cap) {
            raw_log_test_failure_cap *= 2;
            t->failure_reasons =
                realloc(t->failure_reasons,
                        raw_log_test_failure_cap * sizeof *t->failure_reasons);
        }

        raw_log_test_output_t *fail =
            &t->failure_reasons[t->failure_reasons_count++];
        *fail = *out;
        fail->msg = memdup0(buffer, buffer_len);
        fail->file = strdup(file);
        fail->date_time = strdup(ts2);
    }
}

void taf_log_test_started(int index, test_case_t test_case) {
    taf_tui_set_current_test(index, test_case.name);

    if (no_logs) {
        return;
    }

    test_index = index - 1;
    current = test_case;

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
    raw_log_test_output_cap = 2;
    test->output =
        malloc(sizeof(raw_log_test_output_t) * raw_log_test_output_cap);
    test->output_count = 0;

    raw_log_test_failure_cap = 2;
    test->failure_reasons =
        malloc(sizeof(*test->failure_reasons) * raw_log_test_failure_cap);
    test->failure_reasons_count = 0;
}

void taf_log_test_passed(int index, test_case_t test_case) {

    taf_tui_test_passed(index, test_case.name);

    if (no_logs) {
        return;
    }

    char time_str[TS_LEN];
    get_date_time_now(time_str);
    fprintf(output_log_file, "[%s][%s]: Test %d Passed.\n\n", time_str,
            test_case.name, index);

    raw_log_test_t *test = &raw_log->tests[index - 1];
    test->finished = strdup(time_str);
    test->status = "passed";
}

void taf_log_test_failed(int index, test_case_t test_case, const char *msg,
                         const char *file, int line) {

    taf_tui_test_failed(index, test_case.name, msg);

    if (no_logs) {
        return;
    }

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    fprintf(output_log_file, "[%s][%s]: Test %d Failed: %s\n\n", time_str,
            test_case.name, index, msg);

    raw_log_test_t *test = &raw_log->tests[index - 1];
    test->finished = strdup(time_str);
    test->status = "failed";

    if (test->failure_reasons_count >= raw_log_test_failure_cap) {
        raw_log_test_failure_cap *= 2;
        test->failure_reasons =
            realloc(test->failure_reasons,
                    sizeof(*test->failure_reasons) * raw_log_test_failure_cap);
    }

    raw_log_test_output_t *fail_reason =
        &test->failure_reasons[test->failure_reasons_count];
    fail_reason->date_time = strdup(time_str);
    fail_reason->msg = strdup(msg);
    fail_reason->level = TAF_LOG_LEVEL_CRITICAL;
    fail_reason->line = line;
    fail_reason->file = strdup(file);
    test->failure_reasons_count++;
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
        for (size_t k = 0; k < t->failure_reasons_count; ++k) {
            free(t->failure_reasons[k].file);
            free(t->failure_reasons[k].date_time);
            free(t->failure_reasons[k].msg);
        }
        free(t->failure_reasons);

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

    if (no_logs) {
        return;
    }

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
        for (size_t j = 0; j < test->failure_reasons_count; j++) {
            free(test->failure_reasons[j].msg);
            free(test->failure_reasons[j].date_time);
            free(test->failure_reasons[j].file);
        }
        free(test->failure_reasons);
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

static const char *taf_log_level_str_map[] = {"CRITICAL", "ERROR", "WARNING",
                                              "INFO",     "DEBUG", "TRACE"};

const char *taf_log_level_to_str(taf_log_level level) {
    if (level < 0)
        return NULL;
    if (level > TAF_LOG_LEVEL_TRACE)
        return NULL;
    return taf_log_level_str_map[level];
}

taf_log_level taf_log_level_from_str(const char *str) {
    if (!strcasecmp(str, "c") || !strcasecmp(str, "critical"))
        return TAF_LOG_LEVEL_CRITICAL;
    if (!strcasecmp(str, "e") || !strcasecmp(str, "error"))
        return TAF_LOG_LEVEL_ERROR;
    if (!strcasecmp(str, "w") || !strcasecmp(str, "warning"))
        return TAF_LOG_LEVEL_WARNING;
    if (!strcasecmp(str, "i") || !strcasecmp(str, "info"))
        return TAF_LOG_LEVEL_INFO;
    if (!strcasecmp(str, "d") || !strcasecmp(str, "debug"))
        return TAF_LOG_LEVEL_DEBUG;
    if (!strcasecmp(str, "t") || !strcasecmp(str, "trace"))
        return TAF_LOG_LEVEL_TRACE;

    return -1;
}
