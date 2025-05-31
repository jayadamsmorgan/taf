#include "taf_logs.h"

#include "cmd_parser.h"
#include "test_logs.h"

#include "util/files.h"

#include <json.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslimits.h>

static int taf_log_info(cmd_logs_options *opts) {

    if (!file_exists(opts->arg1)) {
        fprintf(stderr, "Log file %s not found.\n", opts->arg1);
        return EXIT_FAILURE;
    }

    json_object *root = json_object_from_file(opts->arg1);
    raw_log_t *raw_log = taf_json_to_raw_log(root);
    if (!raw_log || !raw_log->os || !raw_log->os_version) {
        fprintf(stderr, "Log file %s is either incorrect or corrupt.\n",
                opts->arg1);
        return EXIT_FAILURE;
    }

    printf("TAF test run started on %s\n", raw_log->started);
    printf("TAF version %s\n", raw_log->taf_version);
    printf("Test run performed on %s\n", raw_log->os_version);
    if (raw_log->tags_count != 0) {
        printf("Test run performed with tags [");
        for (size_t i = 0; i < raw_log->tags_count; i++) {
            printf(" '%s'", raw_log->tags[i]);
            if (i != raw_log->tags_count - 1) {
                printf(",");
            }
        }
        printf(" ]\n");
    } else {
        printf("Test run performed with no tags\n");
    }
    printf("Total tests perfomed: %zu\n", raw_log->tests_count);
    size_t passed = 0;
    for (size_t i = 0; i < raw_log->tests_count; i++) {
        raw_log_test_t *test = &raw_log->tests[i];
        printf("Test [%zu] '%s':\n", i + 1, test->name);
        printf("    Tags: [ ");
        for (size_t j = 0; j < test->tags_count; j++) {
            printf(" '%s'", test->tags[j]);
            if (j != test->tags_count - 1) {
                printf(",");
            }
        }
        printf(" ]\n");
        printf("    Started: %s\n", test->started);
        printf("    Finished: %s\n", test->finished);
        printf("    Status: %s\n", test->status);
        if (!strcmp("passed", test->status)) {
            passed++;
        } else if (test->failure_reason) {
            printf("    Failure reason: %s\n", test->failure_reason);
        }
    }

    printf("Total tests passed: %zu\n", passed);
    printf("Total tests failed: %zu\n", raw_log->tests_count - passed);
    printf("Test run finished on %s\n", raw_log->finished);

    return EXIT_SUCCESS;
}

int taf_logs() {

    cmd_logs_options *opts = cmd_parser_get_logs_options();

    switch (opts->category) {
    case LOGS_OPT_INFO:
        return taf_log_info(opts);
    }

    return EXIT_FAILURE;
}
