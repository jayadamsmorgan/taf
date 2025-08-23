#ifndef HEADLESS_H
#define HEADLESS_H

#include "taf_state.h"

void taf_headless_init();

void taf_headless_log_test(taf_state_test_output_t *output);

void taf_headless_test_started(taf_state_test_t *test);
void taf_headless_test_passed(taf_state_test_t *test);
void taf_headless_test_failed(taf_state_test_t *test);

void taf_headless_defer_queue_started(taf_state_test_t *test);
void taf_headless_defer_queue_finished(taf_state_test_t *test);
void taf_headless_defer_queue_failed(taf_state_test_output_t *output);

void taf_headless_hooks_started();
void taf_headless_hooks_finished();
void taf_headless_hook_failed(const char *err);

void taf_headless_finalize();

#endif // HEADLESS_H
