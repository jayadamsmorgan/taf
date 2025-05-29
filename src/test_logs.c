#include "test_logs.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "taf_tui.h"
#include "util/files.h"
#include "util/time.h"
#include "version.h"

#include <json.h>

static test_case_t current = {0};

static FILE *output_log_file;

static char logs_dir[PATH_MAX];
static char output_log_file_path[PATH_MAX];
static char raw_log_file_path[PATH_MAX];

static json_object *raw_log_root;   // Root
static json_object *tests_array;    // Root->tests
static json_object *current_json;   // Root->tests[index]
static json_object *current_output; // Root->tests[index]->output

static json_object *test_case_to_json(test_case_t *tc) {
    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "name", json_object_new_string(tc->name));
    json_object *tag_arr = json_object_new_array();
    for (size_t i = 0; i < tc->tags.amount; i++) {
        json_object_array_add(tag_arr,
                              json_object_new_string(tc->tags.tags[i]));
    }
    json_object_object_add(obj, "tags", tag_arr);
    return obj;
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

    raw_log_root = json_object_new_object();
    json_object *taf_version = json_object_new_string(TAF_VERSION);
    json_object_object_add(raw_log_root, "taf_version", taf_version);
    json_object *date_time = json_object_new_string(time_str);
    json_object_object_add(raw_log_root, "date_time", date_time);
    json_object *tags = json_object_new_array();
    for (size_t i = 0; i < opts->tags_amount; i++) {
        json_object_array_add(tags, json_object_new_string(opts->tags[i]));
    }
    json_object_object_add(raw_log_root, "tags", tags);
    tests_array = json_object_new_array();
    json_object_object_add(raw_log_root, "tests", tests_array);
}

void taf_log_test(const char *file, int line, const char *buffer) {
    taf_tui_log(file, line, buffer);

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    fprintf(output_log_file, "[%s][%s][%s:%d]: %s\n\n", time_str, current.name,
            file, line, buffer);

    json_object *output_obj = json_object_new_object();
    json_object_object_add(output_obj, "file", json_object_new_string(file));
    json_object_object_add(output_obj, "line", json_object_new_int(line));
    json_object_object_add(output_obj, "date_time",
                           json_object_new_string(time_str));
    json_object_object_add(output_obj, "msg", json_object_new_string(buffer));
    json_object_array_add(current_output, output_obj);
}

void taf_log_test_started(int index, test_case_t test_case) {
    current = test_case;
    taf_tui_set_current_test(index, test_case.name);

    char time_str[TS_LEN];
    get_date_time_now(time_str);
    fprintf(output_log_file, "[%s][%s]: Test %d Started.\n\n", time_str,
            test_case.name, index);

    current_json = test_case_to_json(&current);
    json_object_object_add(current_json, "started",
                           json_object_new_string(time_str));
    current_output = json_object_new_array();
    json_object_object_add(current_json, "output", current_output);
}

void taf_log_test_passed(int index, test_case_t test_case) {

    taf_tui_test_passed(index, test_case.name);

    char time_str[TS_LEN];
    get_date_time_now(time_str);
    fprintf(output_log_file, "[%s][%s]: Test %d Passed.\n\n", time_str,
            test_case.name, index);

    json_object_object_add(current_json, "finished",
                           json_object_new_string(time_str));
    json_object_object_add(current_json, "status",
                           json_object_new_string("passed"));
    json_object_array_add(tests_array, current_json);
}

void taf_log_test_failed(int index, test_case_t test_case, const char *msg) {

    taf_tui_test_failed(index, test_case.name, msg);

    char time_str[TS_LEN];
    get_date_time_now(time_str);

    fprintf(output_log_file, "[%s][%s]: Test %d Failed: %s\n\n", time_str,
            test_case.name, index, msg);

    json_object_object_add(current_json, "finished",
                           json_object_new_string(time_str));
    json_object_object_add(current_json, "status",
                           json_object_new_string("failed"));
    json_object_object_add(current_json, "failure_reason",
                           json_object_new_string(msg));
    json_object_array_add(tests_array, current_json);
}

void taf_log_tests_finalize() {

    if (json_object_to_file_ext(raw_log_file_path, raw_log_root,
                                JSON_C_TO_STRING_SPACED |
                                    JSON_C_TO_STRING_PRETTY |
                                    JSON_C_TO_STRING_NOSLASHESCAPE) == -1) {
        // TODO: internal log: Error saving
    }

    // Cleanup
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
