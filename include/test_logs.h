#ifndef TEST_LOGS_H
#define TEST_LOGS_H

#include "test_case.h"

#include <json.h>

typedef struct {
    char *file;
    int line;
    char *date_time;
    char *msg;
} raw_log_test_output_t;

typedef struct {
    char *name;
    char *started;
    char *finished;
    char *status;
    char *failure_reason;

    char **tags;
    size_t tags_count;

    raw_log_test_output_t *output;
    size_t output_count;
} raw_log_test_t;

typedef struct {
    char *taf_version;
    char *os;
    char *os_version;
    char *started;
    char *finished;

    char **tags;
    size_t tags_count;

    raw_log_test_t *tests;
    size_t tests_count;
} raw_log_t;

json_object *taf_raw_log_to_json(raw_log_t *log);
raw_log_t *taf_json_to_raw_log(json_object *obj);

void taf_log_tests_create(int amount);

void taf_log_test(const char *file, int line, const char *buffer);

void taf_log_test_started(int index, test_case_t test_case);

void taf_log_test_passed(int index, test_case_t test_case);

void taf_log_test_failed(int index, test_case_t test_case, const char *msg);

void taf_log_tests_finalize();

#endif // TEST_LOGS_H
