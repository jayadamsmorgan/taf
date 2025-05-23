#ifndef TAF_TUI_H
#define TAF_TUI_H

int taf_tui_init();
void taf_tui_deinit();

void taf_tui_update();

void taf_tui_set_test_amount(int amount);

void taf_tui_set_test_progress(int progress);

void taf_tui_set_current_test(int index, const char *test);

void taf_tui_set_current_line(const char *file, int line, const char *line_str);

void taf_tui_log(const char *file, int line, const char *str);
void taf_tui_test_passed(int index, const char *name);
void taf_tui_test_failed(int index, const char *name, const char *msg);

#endif // TAF_TUI_H
