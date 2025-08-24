#ifndef TAF_TUI_H
#define TAF_TUI_H

#include "taf_state.h"

// UI initialization
int taf_tui_init(taf_state_t *state);

// UI deinitialization
void taf_tui_deinit();

// UI update function; should be called every time pannel should be updated
void taf_tui_update();

void taf_tui_set_test_progress(double progress);

void taf_tui_set_current_line(const char *file, int line, const char *line_str);

void taf_tui_hooks_started();

void taf_tui_hooks_finished();

void taf_tui_hook_failed();

#endif // TAF_TUI_H
