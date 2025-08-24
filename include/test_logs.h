#ifndef TEST_LOGS_H
#define TEST_LOGS_H

#include "taf_state.h"

char *taf_log_get_logs_dir();

char *taf_log_get_output_log_file_path();

char *taf_log_get_raw_log_file_path();

void taf_log_init(taf_state_t *state);

#endif // TEST_LOGS_H
