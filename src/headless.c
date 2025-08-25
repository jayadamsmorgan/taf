#include "headless.h"

#include "cmd_parser.h"
#include "taf_hooks.h"
#include "util/time.h"
#include "version.h"

#include <stdio.h>

static taf_state_t *taf_state = NULL;

static taf_log_level level;
static bool no_logs;

static inline void print_delim(FILE *out) {
    //
    fputs("-----------\n", out);
}

static void taf_headless_test_started(taf_state_test_t *test) {
    printf("Test '%s' STARTED...\n", test->name);
    size_t tags_amount = da_size(test->tags);
    if (tags_amount != 0) {
        char **tag = da_get(test->tags, 0);
        printf("Test tags: '%s'", *tag);
        for (size_t i = 1; i < tags_amount; i++) {
            tag = da_get(test->tags, i);
            printf(", '%s'", *tag);
        }
        printf("\n");
    }
    print_delim(stdout);
}

static void taf_headless_log_test(taf_state_test_t *,
                                  taf_state_test_output_t *output) {
    if (output->level > level) {
        return;
    }
    printf("%s [%s]:\n", output->date_time,
           taf_log_level_to_str(output->level));
    printf("(%s:%d):\n", output->file, output->line);
    printf("%.*s\n", (int)output->msg_len, output->msg);
    print_delim(stdout);
}

static void taf_headless_test_finished(taf_state_test_t *test) {
    if (test->status == TEST_STATUS_FAILED) {
        fprintf(stderr, "%s Test '%s' FAILED:\n", test->finished, test->name);
        size_t size = da_size(test->failure_reasons);
        for (size_t i = 0; i < size; i++) {
            taf_state_test_output_t *o = da_get(test->failure_reasons, i);
            fprintf(stderr, "\nFailure reason %zu: [%s]:\n", i + 1,
                    taf_log_level_to_str(o->level));
            fprintf(stderr, "(%s:%d):\n%.*s\n", o->file, o->line,
                    (int)o->msg_len, o->msg);
        }
    } else if (test->status == TEST_STATUS_PASSED) {
        printf("%s Test '%s' PASSED\n", test->finished, test->name);
    }
    print_delim(stdout);
}

static void taf_headless_defer_queue_started(taf_state_test_t *test) {
    printf("%s Defer Queue for Test '%s' STARTED...\n", test->teardown_start,
           test->name);
    print_delim(stdout);
}

static void taf_headless_defer_failed(taf_state_test_t *,
                                      taf_state_test_output_t *output) {
    fprintf(stderr, "%s Defer (%s:%d) failed. Traceback:\n%.*s\n",
            output->date_time, output->file, output->line, (int)output->msg_len,
            output->msg);
    print_delim(stderr);
}

static void taf_headless_defer_queue_finished(taf_state_test_t *test) {
    printf("%s Defer Queue for Test '%s' FINISHED\n", test->teardown_end,
           test->name);
    print_delim(stdout);
}

static void taf_headless_hook_started(taf_hook_fn) {
    char time[TS_LEN];
    get_date_time_now(time);

    printf("%s Running TAF hooks...\n", time);
    print_delim(stdout);
}

static void taf_headless_hook_finished(taf_hook_fn) {
    char time[TS_LEN];
    get_date_time_now(time);

    printf("%s Finished running TAF hooks.\n", time);
    print_delim(stdout);
}

static void taf_headless_hook_failed(taf_hook_fn, const char *err) {
    char time[TS_LEN];
    get_date_time_now(time);

    fprintf(stderr, "%s TAF hook failed. Traceback:\n%s\n", time, err);
    print_delim(stderr);
}

static void taf_headless_run_started() {

    printf("TAF v" TAF_VERSION " started (headless mode).\n");
    printf("Starting project '%s'.\n", taf_state->project_name);
    if (taf_state->target) {
        printf("Test target selected: '%s'.\n", taf_state->target);
    }
    printf("Output log level: %s\n", taf_log_level_to_str(level));
    size_t tags_amount = da_size(taf_state->tags);
    if (tags_amount != 0) {
        char **tag = da_get(taf_state->tags, 0);
        printf("Test run tags selected: '%s'", *tag);
        for (size_t i = 1; i < tags_amount; i++) {
            tag = da_get(taf_state->tags, i);
            printf(", '%s'", *tag);
        }
        printf("\n");
    }
    if (no_logs) {
        printf("Omitting file logs for this test run.\n");
    }
    print_delim(stdout);
}

static void taf_headless_run_finished() {
    puts("\nTAF Test Run Finished.\n");
    printf("Total: %zu, Passed: %zu, Failed: %zu\n", taf_state->total_amount,
           taf_state->passed_amount, taf_state->failed_amount);
}

void taf_headless_init(taf_state_t *state) {

    cmd_test_options *opts = cmd_parser_get_test_options();

    level = opts->log_level;
    no_logs = opts->no_logs;
    taf_state = state;

    taf_state_register_test_run_started_cb(state, taf_headless_run_started);
    taf_state_register_test_run_finished_cb(state, taf_headless_run_finished);
    taf_state_register_test_started_cb(state, taf_headless_test_started);
    taf_state_register_test_finished_cb(state, taf_headless_test_finished);
    taf_state_register_test_teardown_started_cb(
        state, taf_headless_defer_queue_started);
    taf_state_register_test_defer_failed_cb(state, taf_headless_defer_failed);
    taf_state_register_test_teardown_finished_cb(
        state, taf_headless_defer_queue_finished);
    taf_state_register_test_log_cb(state, taf_headless_log_test);

    taf_hooks_register_hook_started_cb(taf_headless_hook_started);
    taf_hooks_register_hook_finished_cb(taf_headless_hook_finished);
    taf_hooks_register_hook_failed_cb(taf_headless_hook_failed);
}
