#include "test_logs.h"

#include "cmd_parser.h"
#include "headless.h"
#include "internal_logging.h"
#include "project_parser.h"
#include "taf_state.h"
#include "taf_test.h"
#include "taf_tui.h"

#include "util/files.h"
#include "util/time.h"

#include <json.h>

#include <string.h>

static test_case_t current = {0};

static bool no_logs = false;
static taf_log_level log_level;

static FILE *output_log_file;

static char *logs_dir;
static char *output_log_file_path;
static char *raw_log_file_path;

static int test_index = -1;
static bool is_teardown = false;

static bool headless = false;

static taf_state_t *taf_state = NULL;

char *taf_log_get_logs_dir() {
    //
    return logs_dir;
}

char *taf_log_get_output_log_file_path() {
    //
    return output_log_file_path;
}

char *taf_log_get_raw_log_file_path() {
    //
    return raw_log_file_path;
}

int taf_log_get_test_index() {
    //
    return test_index;
}

taf_state_t *taf_log_get_state() {
    //
    return taf_state;
}

void taf_log_tests_create(int amount) {

    LOG("Starting TAF test logging...");

    cmd_test_options *opts = cmd_parser_get_test_options();
    if (opts->no_logs) {
        LOG("No logs option turned on, skipping TAF test logging.");
        no_logs = true;
    }

    headless = opts->headless;
    if (!opts->headless) {
        taf_tui_set_test_amount(amount);
    }

    log_level = opts->log_level;
    LOG("Log level: %s", taf_log_level_to_str(log_level));

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    project_parsed_t *proj = get_parsed_project();

    if (!no_logs) {
        if (proj->multitarget) {
            asprintf(&logs_dir, "%s/logs/%s", proj->project_path, opts->target);
        } else {
            asprintf(&logs_dir, "%s/logs", proj->project_path);
        }
        LOG("Logs directory path: %s", logs_dir);
        if (!directory_exists(logs_dir)) {
            LOG("Logs directory doesn't exist, creating...");
            if (create_directory(logs_dir, MKDIR_MODE)) {
                LOG("Unable to create logs directory.");
                internal_logging_deinit();
                exit(EXIT_FAILURE);
            }
        }

        asprintf(&raw_log_file_path, "%s/test_run_%s_raw.json", logs_dir,
                 time_str);

        LOG("Raw log path: %s", raw_log_file_path);
        asprintf(&output_log_file_path, "%s/test_run_%s_output.log", logs_dir,
                 time_str);
        LOG("Output log path: %s", output_log_file_path);
        output_log_file = fopen(output_log_file_path, "w");
        if (!output_log_file) {
            LOG("Unable to create output log file.");
            internal_logging_deinit();
            exit(EXIT_FAILURE);
        }
        LOG("Created output log file.");
    }

    LOG("Initializing raw log...");
    taf_state = taf_state_new();

    LOG("Raw log initialized.");

    LOG("Successfully started TAF test logging.");
}

void taf_log_test(taf_log_level level, const char *file, int line,
                  const char *buffer, size_t buffer_len) {

    LOG("TAF logging test: log_level '%s', file '%s', line %d, buffer: '%.*s', "
        "buffer_len: %zu",
        taf_log_level_to_str(log_level), file, line, (int)buffer_len, buffer,
        buffer_len);

    char ts[TS_LEN];
    get_date_time_now(ts);

    if (!headless) {
        taf_tui_log(ts, level, file, line, buffer, buffer_len);
    }

    if (level <= log_level && !no_logs) {
        LOG("Writing to output log file...");

        fprintf(output_log_file, "[%s][%s][%s][%s:%d]: ", ts,
                taf_log_level_to_str(level), current.name, file, line);

        fwrite(buffer, 1, buffer_len, output_log_file);
        fputs("\n\n", output_log_file);
        LOG("Wrote to output log file.");
    }

    LOG("Adding raw log test output...");
    taf_state_test_t *t = &taf_state->tests[test_index];

    taf_state_test_output_t out = {
        .level = level,
        .msg = strndup(buffer, buffer_len),
        .msg_len = buffer_len,
        .file = strdup(file),
        .line = line,
        .date_time = strdup(ts),
    };
    if (is_teardown) {
        da_append(t->teardown_outputs, &out);
    } else {
        da_append(t->outputs, &out);
    }

    if (headless) {
        taf_headless_log_test(&out);
    }

    if (level == TAF_LOG_LEVEL_ERROR && !is_teardown) {
        LOG("Adding failure reason...");

        taf_state_test_output_t out = {
            .level = level,
            .msg = strndup(buffer, buffer_len),
            .msg_len = buffer_len,
            .file = strdup(file),
            .line = line,
            .date_time = strdup(ts),
        };
        da_append(t->failure_reasons, &out);

        taf_mark_test_failed();
    }

    LOG("Successfully TAF logged.");
}

void taf_log_test_started(int index, test_case_t test_case) {

    LOG("TAF Logging test '%s' started with index %d...", test_case.name,
        index);

    if (!headless) {
        taf_tui_set_current_test(index, test_case.name);
    }

    test_index = index - 1;
    current = test_case;

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    if (!no_logs) {
        fprintf(output_log_file, "[%s][%s]: Test %d Started.\n\n", time_str,
                test_case.name, index);
        LOG("Wrote to output log file");
    }

    taf_state_test_t *test = &taf_state->tests[index - 1];
    test->started = strdup(time_str);
    test->teardown_start = NULL;
    test->status = NULL;
    test->finished = NULL;

    size_t tags_count = da_size(test_case.tags);
    test->tags = da_init(tags_count, sizeof(char *));
    for (size_t i = 0; i < tags_count; ++i) {
        char **tag = da_get(test_case.tags, i);
        char *to_cpy = strdup(*tag);
        da_append(test->tags, &to_cpy);
    }

    test->name = strdup(test_case.name);
    test->description = test_case.desc ? strdup(test_case.desc) : NULL;

    test->outputs = da_init(1, sizeof(taf_state_test_output_t));
    test->teardown_outputs = da_init(1, sizeof(taf_state_test_output_t));
    test->teardown_errors = da_init(1, sizeof(taf_state_test_output_t));
    test->failure_reasons = da_init(1, sizeof(taf_state_test_output_t));

    if (headless) {
        taf_headless_test_started(test);
    }

    LOG("Successfully TAF logged starting of a test.");
}

void taf_log_test_passed(int index, test_case_t test_case) {

    LOG("TAF logging test '%s' passed at index %d...", test_case.name, index);

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    if (!headless) {
        taf_tui_test_passed(time_str);
    }

    if (!no_logs) {
        fprintf(output_log_file, "[%s][%s]: Test %d Passed.\n\n", time_str,
                test_case.name, index);
        LOG("Wrote to output log file");
    }

    taf_state_test_t *test = &taf_state->tests[index - 1];
    test->finished = strdup(time_str);
    test->status = strdup("passed");

    if (headless) {
        taf_headless_test_passed(test);
    }

    LOG("Successfully TAF logged passing of a test.");
}

void taf_log_test_failed(int index, test_case_t test_case, const char *msg,
                         const char *file, int line) {

    LOG("TAF logging test '%s' failed at index %d", test_case.name, index);

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    taf_state_test_t *test = &taf_state->tests[index - 1];
    test->finished = strdup(time_str);
    test->status = strdup("failed");

    if (msg && file) {
        taf_state_test_output_t fail_reason = {
            .date_time = strdup(time_str),
            .msg = strdup(msg),
            .msg_len = strlen(msg),
            .level = TAF_LOG_LEVEL_CRITICAL,
            .line = line,
            .file = strdup(file),
        };
        da_append(test->failure_reasons, &fail_reason);
    }

    if (!headless) {
        taf_tui_test_failed(time_str, test->failure_reasons);
    } else {
        taf_headless_test_failed(test);
    }

    if (!no_logs) {
        fprintf(output_log_file, "[%s][%s]: Test %d Failed.\n\n", time_str,
                test_case.name, index);
        LOG("Wrote to output log file");
    }

    LOG("Successfully TAF logged test failed.");
}

void taf_log_defer_queue_started() {
    LOG("TAF logging start of the defer queue...");

    is_teardown = true;

    char time[TS_LEN];
    get_date_time_now(time);

    taf_state_test_t *test = &taf_state->tests[test_index];
    test->teardown_start = strdup(time);

    if (!headless) {
        taf_tui_defer_queue_started(time);
    }
    if (headless) {
        taf_headless_defer_queue_started(test);
    }

    if (!no_logs) {
        fprintf(output_log_file, "[%s][%s]: Defer Queue Started.\n\n", time,
                test->name);
        LOG("Wrote to output log file");
    }

    LOG("Successfully TAF logged start of defer queue.");
}

void taf_log_defer_queue_finished() {
    LOG("TAF logging finish of the defer queue...");

    is_teardown = false;

    taf_state_test_t *test = &taf_state->tests[test_index];

    char time[TS_LEN];
    get_date_time_now(time);

    if (!headless) {
        taf_tui_defer_queue_finished(time);
    }
    if (headless) {
        taf_headless_defer_queue_finished(test);
    }

    if (!no_logs) {
        fprintf(output_log_file, "[%s][%s]: Defer Queue Finished.\n\n", time,
                test->name);
        LOG("Wrote to output log file");
    }

    LOG("Successfully TAF logged finish of defer queue.");
}

void taf_log_defer_failed(const char *trace, const char *file, int line) {
    LOG("TAF logging defer failure...");

    taf_state_test_t *test = &taf_state->tests[test_index];

    char time[TS_LEN];
    get_date_time_now(time);

    if (!headless) {
        taf_tui_defer_failed(time, trace, file, line);
    }

    if (!no_logs) {
        fprintf(output_log_file,
                "[%s][%s]: Defer failed (%s at %d), traceback: \n%s\n\n", time,
                test->name, file, line, trace);
        LOG("Wrote to output log file");
    }

    taf_state_test_output_t teardown_err = {
        .date_time = strdup(time),
        .msg = strdup(trace),
        .msg_len = strlen(trace),
        .level = TAF_LOG_LEVEL_CRITICAL,
        .line = line,
        .file = strdup(file),
    };
    da_append(test->teardown_errors, &teardown_err);

    if (headless) {
        taf_headless_defer_queue_failed(&teardown_err);
    }

    LOG("Successfully TAF logged defer failure.");
}

void taf_log_tests_finalize() {

    LOG("Finalizing TAF logging...");

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    taf_state->finished = strdup(time_str);

    if (headless) {
        taf_headless_finalize();
    }

    if (!no_logs) {
        json_object *taf_state_root = taf_state_to_json(taf_state);

        LOG("Saving raw log file...");
        if (json_object_to_file_ext(raw_log_file_path, taf_state_root,
                                    JSON_C_TO_STRING_SPACED |
                                        JSON_C_TO_STRING_PRETTY |
                                        JSON_C_TO_STRING_NOSLASHESCAPE) == -1) {
            LOG("Unable to save raw log file: %s", json_util_get_last_err());
        }
        LOG("Freeing JSON object...");
        json_object_put(taf_state_root);
    }

    // Cleanup
    LOG("Freeing taf state...");
    taf_state_free(taf_state);

    char *latest_log = NULL;
    char *latest_raw = NULL;

    if (!no_logs) {
        LOG("Flushing and closing output log file...");
        fflush(output_log_file);
        fclose(output_log_file);

        // Create 'latest' symlinks:
        project_parsed_t *proj = get_parsed_project();
        if (proj->multitarget) {
            asprintf(&latest_raw, "%s/test_run_latest_raw.json", logs_dir);
        } else {
            asprintf(&latest_log, "%s/test_run_latest_output.log", logs_dir);
            asprintf(&latest_raw, "%s/test_run_latest_raw.json", logs_dir);
        }

        LOG("Creating symlinks '%s' and '%s'...", latest_log, latest_raw);
        replace_symlink(output_log_file_path, latest_log);
        replace_symlink(raw_log_file_path, latest_raw);

        if (proj->multitarget) {
            free(latest_log);
            asprintf(&latest_log, "%s/logs/test_run_latest_output.log",
                     proj->project_path);
            free(latest_raw);
            asprintf(&latest_raw, "%s/logs/test_run_latest_raw.json",
                     proj->project_path);
            replace_symlink(output_log_file_path, latest_log);
            replace_symlink(raw_log_file_path, latest_raw);
        }
    }

    free(output_log_file_path);
    free(raw_log_file_path);
    free(logs_dir);
    free(latest_raw);

    LOG("Successfully finalized TAF logging.");
}
