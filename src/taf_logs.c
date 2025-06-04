#include "taf_logs.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "test_logs.h"

#include "util/files.h"

#include <json.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslimits.h>

int taf_logs_info() {

    cmd_logs_info_options *opts = cmd_parser_get_logs_info_options();

    char log_file_path[PATH_MAX];

    if (!strcmp(opts->arg, "latest")) {
        if (project_parser_parse()) {
            return EXIT_FAILURE;
        }
        project_parsed_t *proj = get_parsed_project();
        snprintf(log_file_path, PATH_MAX, "%s/logs/test_run_latest_raw.json",
                 proj->project_path);
    } else if (file_exists(opts->arg)) {
        snprintf(log_file_path, PATH_MAX, "%s", opts->arg);
    } else {
        fprintf(stderr, "Log file %s not found.\n", opts->arg);
        return EXIT_FAILURE;
    }

    json_object *root = json_object_from_file(log_file_path);
    raw_log_t *raw_log = taf_json_to_raw_log(root);
    if (!raw_log || !raw_log->os || !raw_log->os_version) {
        fprintf(stderr, "Log file %s is either incorrect or corrupt.\n",
                log_file_path);
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
    printf("Total tests perfomed: %zu\n\n", raw_log->tests_count);
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
        } else if (test->failure_reasons_count != 0) {
            printf("    Failure reasons:\n");
            for (size_t i = 0; i < test->failure_reasons_count; i++) {
                raw_log_test_output_t *failure = &test->failure_reasons[i];
                printf("        %zu: [%s]: %s\n", i + 1,
                       taf_log_level_to_str(failure->level), failure->msg);
            }
        }
        printf("\n");
    }

    printf("Total tests passed: %zu\n", passed);
    printf("Total tests failed: %zu\n", raw_log->tests_count - passed);
    printf("Test run finished on %s\n", raw_log->finished);

    return EXIT_SUCCESS;
}
