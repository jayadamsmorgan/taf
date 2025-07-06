#ifndef TAF_TUI_H
#define TAF_TUI_H

#include "test_logs.h"

int taf_tui_init();
void taf_tui_deinit();

void taf_tui_update();

void taf_tui_set_test_amount(int amount);

void taf_tui_set_test_progress(double progress);

void taf_tui_set_current_test(int index, const char *test);

void taf_tui_set_current_line(const char *file, int line, const char *line_str);

void taf_tui_log(char *time, taf_log_level log_level, const char *file,
                 int line, const char *buffer, size_t buffer_len);
void taf_tui_test_passed(char *time);
void taf_tui_test_failed(char *time, raw_log_test_output_t *failure_reasons,
                         size_t failure_reasons_count);

void taf_tui_defer_queue_started(char *time);
void taf_tui_defer_queue_finished(char *time);

void taf_tui_defer_failed(char *time, const char *trace, const char *file,
                          int line);

#endif // TAF_TUI_H
