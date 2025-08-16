#include "headless.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "util/time.h"
#include "version.h"

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
    if (opts->tags_amount != 0) {
        printf("Test run tags selected: '%s'", opts->tags[0]);
        for (size_t i = 1; i < opts->tags_amount; i++) {
            printf(", '%s'", opts->tags[i]);
        }
        printf("\n");
    }
    if (opts->no_logs) {
        printf("Omitting file logs for this test run.\n");
    }
    print_delim(stdout);
}

void taf_headless_test_started(raw_log_test_t *test) {
    printf("Test '%s' STARTED...\n", test->name);
    if (test->tags_count != 0) {
        printf("Test tags: '%s'", test->tags[0]);
        for (size_t i = 1; i < test->tags_count; i++) {
            printf(", %s", test->tags[i]);
        }
        printf("\n");
    }
    print_delim(stdout);
    test_amount++;
}

void taf_headless_log_test(raw_log_test_output_t *output) {
    printf("%s [%s]:\n", output->date_time,
           taf_log_level_to_str(output->level));
    printf("(%s:%d):\n", output->file, output->line);
    printf("%.*s\n", (int)output->msg_len, output->msg);
    print_delim(stdout);
}

void taf_headless_test_failed(raw_log_test_t *test) {
    fprintf(stderr, "%s Test '%s' FAILED:\n", test->finished, test->name);
    for (size_t i = 0; i < test->failure_reasons_count; i++) {
        raw_log_test_output_t *o = &test->failure_reasons[i];
        fprintf(stderr, "\nFailure reason %zu: [%s]:\n", i + 1,
                taf_log_level_to_str(o->level));
        fprintf(stderr, "(%s:%d):\n%.*s\n", o->file, o->line, (int)o->msg_len,
                o->msg);
    }
    print_delim(stderr);
}

void taf_headless_test_passed(raw_log_test_t *test) {
    printf("%s Test '%s' PASSED\n", test->finished, test->name);
    print_delim(stdout);
    passed_amount++;
}

void taf_headless_defer_queue_started(raw_log_test_t *test) {
    char time[TS_LEN];
    get_date_time_now(time);

    printf("%s Defer Queue for Test '%s' STARTED...\n", time, test->name);
    print_delim(stdout);
}

void taf_headless_defer_queue_failed(raw_log_test_output_t *output) {
    fprintf(stderr, "%s Defer (%s:%d) failed. Traceback:\n%.*s\n",
            output->date_time, output->file, output->line, (int)output->msg_len,
            output->msg);
    print_delim(stderr);
}

void taf_headless_defer_queue_finished(raw_log_test_t *test) {
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
