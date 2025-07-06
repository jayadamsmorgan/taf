#ifndef TEST_LOGS_H
#define TEST_LOGS_H

#include "test_case.h"

#include <json.h>

typedef enum {
    TAF_LOG_LEVEL_CRITICAL = 0,
    TAF_LOG_LEVEL_ERROR = 1,
    TAF_LOG_LEVEL_WARNING = 2,
    TAF_LOG_LEVEL_INFO = 3,
    TAF_LOG_LEVEL_DEBUG = 4,
    TAF_LOG_LEVEL_TRACE = 5,
} taf_log_level;

typedef struct {
    char *file;
    int line;
    char *date_time;
    taf_log_level level;
    char *msg;
    size_t msg_len;
} raw_log_test_output_t;

typedef struct {
    char *name;
    char *started;
    char *finished;
    char *teardown_start;
    char *status;

    raw_log_test_output_t *failure_reasons;
    size_t failure_reasons_count;

    char **tags;
    size_t tags_count;

    raw_log_test_output_t *outputs;
    size_t outputs_count;

    raw_log_test_output_t *teardown_outputs;
    size_t teardown_outputs_count;

    raw_log_test_output_t *teardown_errors;
    size_t teardown_errors_count;
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

taf_log_level taf_log_level_from_str(const char *str);
const char *taf_log_level_to_str(taf_log_level level);

json_object *taf_raw_log_to_json(raw_log_t *log);
raw_log_t *taf_json_to_raw_log(json_object *obj);

void taf_log_tests_create(int amount);

void taf_log_test(taf_log_level log_level, const char *file, int line,
                  const char *buffer, size_t buffer_len);

void taf_log_test_started(int index, test_case_t test_case);

void taf_log_test_passed(int index, test_case_t test_case);

void taf_log_test_failed(int index, test_case_t test_case, const char *msg,
                         const char *file, int line);

void taf_log_tests_finalize();

void taf_log_defer_queue_started();

void taf_log_defer_queue_finished();

void taf_log_defer_failed(const char *trace, const char *file, int line);

#endif // TEST_LOGS_H
