#ifndef TEST_LOGS_H
#define TEST_LOGS_H

#include "taf_log_level.h"
#include "taf_state.h"
#include "test_case.h"

taf_state_t *taf_log_get_state();

char *taf_log_get_logs_dir();

char *taf_log_get_output_log_file_path();

char *taf_log_get_raw_log_file_path();

int taf_log_get_test_index();

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
