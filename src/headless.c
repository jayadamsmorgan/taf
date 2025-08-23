#include "headless.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "util/time.h"
#include "version.h"

#include <stdio.h>

static inline void print_delim(FILE *out) { fputs("-----------\n", out); }

static size_t passed_amount = 0;
static size_t test_amount = 0;

void taf_headless_init() {
    cmd_test_options *opts = cmd_parser_get_test_options();
    project_parsed_t *proj = get_parsed_project();

    printf("TAF v" TAF_VERSION " started (headless mode).\n");
    printf("Starting project '%s'.\n", proj->project_name);
    printf("Project minimal TAF version: %s\n", proj->min_taf_ver_str);
    if (opts->target) {
        printf("Test target selected: '%s'.\n", opts->target);
    }
    printf("Output log level: %s\n", taf_log_level_to_str(opts->log_level));
    size_t tags_amount = da_size(opts->tags);
    if (tags_amount != 0) {
        char **tag = da_get(opts->tags, 0);
        printf("Test run tags selected: '%s'", *tag);
        for (size_t i = 1; i < tags_amount; i++) {
            tag = da_get(opts->tags, i);
            printf(", '%s'", *tag);
        }
        printf("\n");
    }
    if (opts->no_logs) {
        printf("Omitting file logs for this test run.\n");
    }
    print_delim(stdout);
}

void taf_headless_test_started(taf_state_test_t *test) {
    printf("Test '%s' STARTED...\n", test->name);
    size_t tags_amount = da_size(test->tags);
    if (tags_amount != 0) {
        char **tag = da_get(test->tags, 0);
        printf("Test tags: '%s'", *tag);
        for (size_t i = 1; i < tags_amount; i++) {
            tag = da_get(test->tags, i);
            printf(", %s", *tag);
        }
        printf("\n");
    }
    print_delim(stdout);
    test_amount++;
}

void taf_headless_log_test(taf_state_test_output_t *output) {
    printf("%s [%s]:\n", output->date_time,
           taf_log_level_to_str(output->level));
    printf("(%s:%d):\n", output->file, output->line);
    printf("%.*s\n", (int)output->msg_len, output->msg);
    print_delim(stdout);
}

void taf_headless_test_failed(taf_state_test_t *test) {
    fprintf(stderr, "%s Test '%s' FAILED:\n", test->finished, test->name);
    size_t size = da_size(test->failure_reasons);
    for (size_t i = 0; i < size; i++) {
        taf_state_test_output_t *o = da_get(test->failure_reasons, i);
        fprintf(stderr, "\nFailure reason %zu: [%s]:\n", i + 1,
                taf_log_level_to_str(o->level));
        fprintf(stderr, "(%s:%d):\n%.*s\n", o->file, o->line, (int)o->msg_len,
                o->msg);
    }
    print_delim(stderr);
}

void taf_headless_test_passed(taf_state_test_t *test) {
    printf("%s Test '%s' PASSED\n", test->finished, test->name);
    print_delim(stdout);
    passed_amount++;
}

void taf_headless_defer_queue_started(taf_state_test_t *test) {
    char time[TS_LEN];
    get_date_time_now(time);

    printf("%s Defer Queue for Test '%s' STARTED...\n", time, test->name);
    print_delim(stdout);
}

void taf_headless_defer_queue_failed(taf_state_test_output_t *output) {
    fprintf(stderr, "%s Defer (%s:%d) failed. Traceback:\n%.*s\n",
            output->date_time, output->file, output->line, (int)output->msg_len,
            output->msg);
    print_delim(stderr);
}

void taf_headless_defer_queue_finished(taf_state_test_t *test) {
    char time[TS_LEN];
    get_date_time_now(time);

    printf("%s Defer Queue for Test '%s' FINISHED\n", time, test->name);
    print_delim(stdout);
}

void taf_headless_finalize() {
    puts("\nTAF Test Run Finished.\n");
    printf("Total: %zu, Passed: %zu, Failed: %zu\n", test_amount, passed_amount,
           test_amount - passed_amount);
}

void taf_headless_hooks_started() {
    char time[TS_LEN];
    get_date_time_now(time);

    printf("%s Running TAF hooks...\n", time);
    print_delim(stdout);
}

void taf_headless_hooks_finished() {
    char time[TS_LEN];
    get_date_time_now(time);

    printf("%s Finished running TAF hooks.\n", time);
    print_delim(stdout);
}

void taf_headless_hook_failed(const char *err) {
    char time[TS_LEN];
    get_date_time_now(time);

    fprintf(stderr, "%s TAF hook failed. Traceback:\n%s\n", time, err);
    print_delim(stderr);
}
