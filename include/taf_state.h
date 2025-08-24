#ifndef RAW_LOG_H
#define RAW_LOG_H

#include "taf_log_level.h"
#include "test_case.h"

#include "util/da.h"

#include <json.h>

#include <stddef.h>

typedef struct {
    char *file;
    int line;
    char *date_time;
    taf_log_level level;
    char *msg;
    size_t msg_len;
} taf_state_test_output_t;

typedef enum {
    TEST_STATUS_RUNNING = 0U,
    TEST_STATUS_TEARDOWN_AFTER_FAILED = 1U,
    TEST_STATUS_TEARDOWN_AFTER_PASSED = 2U,
    TEST_STATUS_FAILED = 3U,
    TEST_STATUS_PASSED = 4U,
} taf_state_test_status;

typedef struct {
    char *name;
    char *description;
    char *started;
    char *finished;
    char *teardown_start;
    char *teardown_end;

    taf_state_test_status status;
    char *status_str;

    da_t *tags;

    da_t *failure_reasons;
    da_t *outputs;
    da_t *teardown_outputs;
    da_t *teardown_errors;

} taf_state_test_t;

typedef void (*test_run_cb)();
typedef void (*test_cb)(taf_state_test_t *);
typedef void (*test_log_cb)(taf_state_test_t *, taf_state_test_output_t *);

typedef struct {
    char *project_name;
    char *taf_version;
    char *os;
    char *os_version;
    char *started;
    char *finished;
    char *target;

    size_t total_amount;
    size_t passed_amount;
    size_t failed_amount;
    size_t finished_amount;

    da_t *vars;

    da_t *tags;

    da_t *tests;

    da_t *test_run_started_cbs;       // test_run_cb
    da_t *test_started_cbs;           // test_cb
    da_t *test_finished_cbs;          // test_cb
    da_t *test_log_cbs;               // test_log_cb
    da_t *test_teardown_started_cbs;  // test_cb
    da_t *test_teardown_finished_cbs; // test_cb
    da_t *test_defer_failed_cbs;      // test_log_cb
    da_t *test_run_finished_cbs;      // test_run_cb

} taf_state_t;

json_object *taf_state_to_json(taf_state_t *log);

taf_state_t *taf_state_from_json(json_object *obj);

taf_state_t *taf_state_new();

void taf_state_test_run_started(taf_state_t *state);

void taf_state_test_run_finished(taf_state_t *state);

void taf_state_test_failed(taf_state_t *state, const char *file, int line,
                           const char *msg);

void taf_state_test_passed(taf_state_t *state);

void taf_state_test_defer_queue_finished(taf_state_t *state);

void taf_state_test_defer_failed(taf_state_t *state, const char *file, int line,
                                 const char *msg);

void taf_state_test_defer_queue_started(taf_state_t *state);

void taf_state_test_log(taf_state_t *state, taf_log_level level,
                        const char *file, int line, const char *buffer,
                        size_t buffer_len);

void taf_state_test_started(taf_state_t *state, test_case_t *test_case);

void taf_state_free(taf_state_t *state);

void taf_state_register_test_run_started_cb(taf_state_t *state, test_run_cb cb);
void taf_state_register_test_run_finished_cb(taf_state_t *state,
                                             test_run_cb cb);
void taf_state_register_test_started_cb(taf_state_t *state, test_cb cb);
void taf_state_register_test_finished_cb(taf_state_t *state, test_cb cb);
void taf_state_register_test_log_cb(taf_state_t *state, test_log_cb cb);
void taf_state_register_test_teardown_started_cb(taf_state_t *state,
                                                 test_cb cb);
void taf_state_register_test_teardown_finished_cb(taf_state_t *state,
                                                  test_cb cb);
void taf_state_register_test_defer_failed_cb(taf_state_t *state,
                                             test_log_cb cb);

#endif // RAW_LOG_H
