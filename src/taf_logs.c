#include "taf_logs.h"

#include "internal_logging.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "taf_state.h"

#include "taf_vars.h"
#include "util/files.h"

#include <json.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <sys/syslimits.h>
#else
#include <limits.h>
#endif // __APPLE__

int taf_logs_info() {

    cmd_logs_info_options *opts = cmd_parser_get_logs_info_options();

    if (opts->internal_logging && internal_logging_init()) {
        fprintf(stderr, "Unable to init internal_logging.\n");
        return EXIT_FAILURE;
    }

    LOG("Starting taf logs info...");

    char log_file_path[PATH_MAX];

    if (!strcmp(opts->arg, "latest")) {
        LOG("Getting latest log...");
        if (project_parser_parse()) {
            internal_logging_deinit();
            return EXIT_FAILURE;
        }
        project_parsed_t *proj = get_parsed_project();
        snprintf(log_file_path, PATH_MAX, "%s/logs/test_run_latest_raw.json",
                 proj->project_path);
    } else if (file_exists(opts->arg)) {
        snprintf(log_file_path, PATH_MAX, "%s", opts->arg);
    } else {
        LOG("Log file '%s' not found.", log_file_path);
        fprintf(stderr, "Log file %s not found.\n", opts->arg);
        internal_logging_deinit();
        return EXIT_FAILURE;
    }

    LOG("Log path: %s", log_file_path);

    LOG("Getting json object...");
    json_object *root = json_object_from_file(log_file_path);
    taf_state_t *taf_state = taf_state_from_json(root);
    if (!taf_state || !taf_state->os || !taf_state->os_version) {
        LOG("Log file is incorrect or corrupt");
        fprintf(stderr, "Log file %s is either incorrect or corrupt.\n",
                log_file_path);
        internal_logging_deinit();
        return EXIT_FAILURE;
    }

    printf("TAF test run started on %s\n", taf_state->started);
    printf("TAF version %s\n", taf_state->taf_version);
    printf("Test run performed on %s\n", taf_state->os_version);
    if (taf_state->target) {
        printf("Test target: '%s'\n", taf_state->target);
    }
    size_t tags_count = da_size(taf_state->tags);
    if (tags_count != 0) {
        printf("Test run performed with tags [");
        for (size_t i = 0; i < tags_count; i++) {
            char **tag = da_get(taf_state->tags, i);
            printf(" '%s'", *tag);
            if (i != tags_count - 1) {
                printf(",");
            }
        }
        printf(" ]\n");
    } else {
        printf("Test run performed with no tags\n");
    }
    size_t vars_count = da_size(taf_state->vars);
    if (vars_count != 0) {
        printf("Test run performed with variables:\n");
        for (size_t i = 0; i < vars_count; i++) {
            taf_var_entry_t *e = da_get(taf_state->vars, i);
            printf("    '%s' = '%s'\n", e->name, e->final_value);
        }
    }
    printf("Total tests perfomed: %zu\n\n", taf_state->total_amount);
    for (size_t i = 0; i < taf_state->total_amount; i++) {
        taf_state_test_t *test = &taf_state->tests[i];
        printf("Test [%zu] '%s':\n", i + 1, test->name);
        printf("    Tags: [");
        size_t tags_count = da_size(test->tags);
        for (size_t j = 0; j < tags_count; j++) {
            char **tag = da_get(test->tags, j);
            printf(" '%s'", *tag);
            if (j != tags_count - 1) {
                printf(",");
            }
        }
        printf(" ]\n");
        printf("    Started: %s\n", test->started);
        printf("    Finished: %s\n", test->finished);
        printf("    Status: %s\n", test->status);
        size_t failure_reasons_count = da_size(test->failure_reasons);
        if (failure_reasons_count != 0) {
            printf("    Failure reasons:\n");
            for (size_t j = 0; j < failure_reasons_count; j++) {
                taf_state_test_output_t *failure =
                    da_get(test->failure_reasons, j);
                printf("        %zu: [%s]: %s\n", j + 1,
                       taf_log_level_to_str(failure->level), failure->msg);
            }
        }
        if (opts->include_outputs) {
            size_t outputs_count = da_size(test->outputs);
            if (outputs_count == 0) {
                printf("    No test outputs.\n");
            } else {
                printf("    Outputs:\n");
                for (size_t j = 0; j < outputs_count; j++) {
                    taf_state_test_output_t *output = da_get(test->outputs, j);
                    printf("---------\n");
                    printf("        %zu: [%s][%s]:\n%s\n", j + 1,
                           output->date_time,
                           taf_log_level_to_str(output->level), output->msg);
                    printf("---------\n");
                }
            }
            size_t teardown_outputs_count = da_size(test->teardown_outputs);
            if (teardown_outputs_count == 0) {
                printf("    No teardown outputs.\n");
            } else {
                printf("    Teardown Outputs:\n");
                for (size_t j = 0; j < teardown_outputs_count; j++) {
                    taf_state_test_output_t *output =
                        da_get(test->teardown_outputs, j);
                    printf("---------\n");
                    printf("        %zu: [%s][%s]:\n%s\n", j + 1,
                           output->date_time,
                           taf_log_level_to_str(output->level), output->msg);
                    printf("---------\n");
                }
            }
            size_t teardown_errors_count = da_size(test->teardown_errors);
            if (teardown_errors_count == 0) {
                printf("    No teardown errors.\n");
            } else {
                printf("    Teardown errors:\n");
                for (size_t j = 0; j < teardown_errors_count; j++) {
                    taf_state_test_output_t *output =
                        da_get(test->teardown_errors, j);
                    printf("---------\n");
                    printf("        %zu: [%s][%s]:\n%s\n", j + 1,
                           output->date_time,
                           taf_log_level_to_str(output->level), output->msg);
                    printf("---------\n");
                }
            }
        }
        printf("\n");
    }

    printf("Total tests passed: %zu\n", taf_state->passed_amount);
    printf("Total tests failed: %zu\n", taf_state->failed_amount);
    printf("Test run finished on %s\n", taf_state->finished);

    internal_logging_deinit();

    project_parser_free();

    return EXIT_SUCCESS;
}
