#ifndef TEST_LOGS_H
#define TEST_LOGS_H

#include "test_case.h"

void taf_log_tests_create(int amount);

void taf_log_test(const char *file, int line, const char *buffer);

void taf_log_test_started(int index, test_case_t test_case);

void taf_log_test_passed(int index, test_case_t test_case);

void taf_log_test_failed(int index, test_case_t test_case, const char *msg);

void taf_log_tests_finalize();

#endif // TEST_LOGS_H
