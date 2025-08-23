#ifndef TAF_TUI_H
#define TAF_TUI_H

#include "taf_state.h"

// UI initialization
int taf_tui_init();

// UI deinitialization
void taf_tui_deinit();

// UI update function; should be called every time pannel should be updated
void taf_tui_update();

void taf_tui_log(char *time, taf_log_level log_level, const char *, int,
                 const char *buffer, size_t buffer_len);
void taf_tui_set_test_amount(int amount);
void taf_tui_set_current_test(int index, const char *test);

void taf_tui_set_test_progress(double progress);
void taf_tui_set_current_line(const char *file, int line, const char *line_str);

void taf_tui_defer_queue_started(char *time);

void taf_tui_defer_queue_finished(char *time);

void taf_tui_test_passed(char *time);

void taf_tui_defer_failed(char *time, const char *trace, const char *file,
                          int line);

void taf_tui_test_failed(char *time, da_t *failure_reasons);

void taf_tui_hooks_started(char *time);

void taf_tui_hooks_finished(char *time);

void taf_tui_hook_failed(char *time, const char *trace);
#endif // TAF_TUI_H
