#ifndef HEADLESS_H
#define HEADLESS_H

#include "test_logs.h"

void taf_headless_init();

void taf_headless_log_test(raw_log_test_output_t *output);

void taf_headless_test_started(raw_log_test_t *test);
void taf_headless_test_passed(raw_log_test_t *test);
void taf_headless_test_failed(raw_log_test_t *test);

void taf_headless_defer_queue_started(raw_log_test_t *test);
void taf_headless_defer_queue_finished(raw_log_test_t *test);
void taf_headless_defer_queue_failed(raw_log_test_output_t *output);

void taf_headless_hooks_started();
void taf_headless_hooks_finished();
void taf_headless_hook_failed(const char *err);

void taf_headless_finalize();

#endif // HEADLESS_H
