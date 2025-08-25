#include "taf_tui.h"

#include "cmd_parser.h"
#include "taf_vars.h"
#include "version.h"

#include "internal_logging.h"
#include "util/string.h"
#include "util/time.h"
#include "util/da.h"
#include <picotui.h>

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Possible test result
typedef enum {
    PASSED = 0U,
    FAILED = 1U,
    RUNNING = 2U,
} ui_test_progress_state;

// I have no idea what this is...
typedef struct {

    taf_log_level log_level;

    bool is_teardown;
    bool is_hooking;

    double current_test_progress;
    char *current_file;
    int current_line;
    char *current_line_str;

} ui_state_t;

static ui_state_t ui_state = {0};

static taf_state_t *taf_state = NULL;

static pico_t *ui = NULL;

static int log_level_to_palindex_map[] = {
    9, 1, 3, 4, 2, 6,
};

void taf_tui_set_test_progress(double progress) {
    //
    ui_state.current_test_progress = progress;
}


static size_t sanitize_inplace(char *buf, size_t len) {
    size_t r = 0; /* read cursor */
    size_t w = 0; /* write cursor */

    while (r < len) {
        char c = buf[r];

        /* collapse CRLF → LF */
        if (c == '\r' && r + 1 < len && buf[r + 1] == '\n') {
            buf[w++] = '\n';
            r += 2;
            continue;
        }

        /* turn NUL into space (0x20) */
        if (c == '\0')
            c = ' ';

        buf[w++] = c;
        r++;
    }

    if (w < len) /* keep C-string semantics if we can */
        buf[w] = '\0';

    return w; /* new logical length */
}

char *ll_to_str[] = {
    "CRT", "ERR", "WRN", "INF", "DBG", "TRC",
};

void taf_tui_log(taf_state_test_t *test, taf_state_test_output_t *output) {

    // Filter for logs with lower log level
    if (output->level > ui_state.log_level) {
        return;
    }

    // Allocate space for logs  and remove inapropriate synbols from it
    char *tmp = malloc(output->msg_len + 1);
    memcpy(tmp, output->msg, output->msg_len);
    tmp[output->msg_len] = '\0';
    sanitize_inplace(tmp, output->msg_len);

    // Wrie Log Level information for current run
    pico_set_colors(ui, log_level_to_palindex_map[output->level], -1);
    pico_printf(ui, "[%s]", ll_to_str[output->level]);
    pico_print(ui, "");

    // Write time form test start (time in format hh:mm:ss)
    pico_set_colors(ui, PICO_COLOR_BRIGHT_MAGENTA, -1);
    pico_printf(ui, "(%s)", output->date_time + 9);
    pico_print(ui, " ");

    // Write logs body
    pico_reset_colors(ui);
    pico_print_block(ui, tmp);
    free(tmp);
}


/*------------------- TAF UI functions -------------------*/
static void taf_tui_project_header_render(pico_t *ui)
{

    pico_reset_colors(ui); // better to do it
    
    /* Line 0: Main title */
    pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);
    pico_ui_clear_line(ui, 0);
    pico_ui_puts_yx(ui, 0, 0, "TAF v" TAF_VERSION);

    /* Line 1: | */
    pico_ui_clear_line(ui, 1);
    pico_ui_puts_yx(ui, 1, 0, "│");

    /* Line 2: Project Information */
    pico_ui_clear_line(ui, 2);
    pico_ui_puts_yx(ui, 2, 0, "├─ Project Information:");

    /* Line 3: Project name */
    pico_ui_clear_line(ui, 3);
    pico_ui_printf_yx(ui, 3, 0, "│  ├─ Project: %s", taf_state->project_name);

    /* Line 4: Project name */
    pico_ui_clear_line(ui, 4);
    pico_ui_printf_yx(ui, 4, 0, "│  ├─ Target: %s",
                      taf_state->target ? taf_state->target : "none");

    /* Line 5: Project tags */
    pico_ui_clear_line(ui, 5);
    pico_ui_printf_yx(ui, 5, 0, "│  ├─ Tags: [ ");
    size_t tags_count = da_size(taf_state->tags);
    size_t offset = 14;
    for (size_t i = 0; i < tags_count; ++i) {
        char **tag = da_get(taf_state->tags, i);
        pico_ui_printf_yx(ui, 5, offset, "'%s' ", *tag);
        offset += strlen(*tag) + 3;
    }
    pico_ui_printf_yx(ui, 5, offset, "]");

    /* Line 6: Project vars */
    pico_ui_clear_line(ui, 6);
    pico_ui_printf_yx(ui, 6, 0, "│  ├─ Vars: [ ");
    size_t vars_count = da_size(taf_state->vars);
    offset = 14;
    for (size_t i = 0; i < vars_count; ++i) {
        taf_var_entry_t *e = da_get(taf_state->vars, i);
        pico_ui_printf_yx(ui, 6, offset, "'%s=%s' ", e->name, e->final_value);
        offset += strlen(e->name) + strlen(e->final_value) + 3;
    }
    pico_ui_printf_yx(ui,6, offset, "]");

    /* Line 7: Project Log Level */
    pico_ui_clear_line(ui, 7);
    pico_ui_puts_yx(ui, 7, 0, "│  └─ Log Level: ");
    pico_set_colors(ui, log_level_to_palindex_map[ui_state.log_level], -1);
    pico_ui_printf_yx(ui, 7, 17, "%s",
                      taf_log_level_to_str(ui_state.log_level));

}

static void taf_tui_test_progress_render(pico_t *ui)
{

    pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);

    /* Line 8: Test Progress */
    pico_ui_clear_line(ui, 8);
    pico_ui_puts_yx(ui, 8, 0, "├─ Test Progress:");

    size_t tests_count = da_size(taf_state->tests);
    if (tests_count == 0)
        return;

    taf_state_test_t *test = da_get(taf_state->tests, tests_count - 1);

    /* Line 9: Test Name and millis from the start*/
    pico_ui_clear_line(ui, 9);

    LOG("WOW %s %s", test->name, test->started);
    pico_ui_printf_yx(ui, 9, 0, "│  ├─ Name: %s", test->name);


    /* Line 10: Test Progress */
    pico_ui_clear_line(ui, 10);
    pico_ui_printf_yx(ui, 10, 0, "│  ├─ Progress: %d%%",
                      (unsigned int)(ui_state.current_test_progress * 100));

    pico_ui_printf_yx(ui, 10, strlen(test->name) + 13, "[ %lums ]",
                       millis_since_start());
    
    /* Line 11: Current Line in Test */
    if (test->status==TEST_STATUS_RUNNING) {
        char *file_str = ui_state.current_file;
        if (file_str) {
            size_t len = strlen(ui_state.current_file);
            if (len > 40) {
                file_str += len - 40;
            }
            pico_ui_clear_line(ui, 11);
            pico_ui_printf_yx(ui, 11, 0, "│  └─ Current Line: [%s%s:%d] %s",
                              len > 40 ? "..." : "", file_str,
                              ui_state.current_line, ui_state.current_line_str);
        }
    }


}
static void taf_tui_summary_render(pico_t *ui, size_t size){

    /* Line 12: Test Case Summary */
    pico_ui_clear_line(ui, size);
    pico_ui_puts_yx(ui, size, 0, "└─ Summary:");

    /* Line 13: Test Case Status */
    pico_ui_clear_line(ui, size+1);
    pico_ui_printf_yx(
        ui, size+1, 0,
        "   ├─ Test Case Status: Total: %zu | Passsed: %zu | Failed: %zu",
        taf_state->total_amount, taf_state->passed_amount,
        taf_state->failed_amount);

    /* Line 14: Test Elapsed Time */
    uint64_t ms;
        ms = millis_since_taf_start();
    const unsigned long minutes = ms / 60000;
    const unsigned long seconds = (ms / 1000) % 60;
    const unsigned long millis = ms % 1000;

    pico_ui_clear_line(ui, size+2);
    pico_ui_printf_yx(ui, size+2, 0, "   └─ Elapsed Time: %lum %lu.%03lus ",
                      minutes, seconds, millis);
}
static void taf_tui_test_run_result(pico_t *ui, size_t tests_count){

    pico_set_colors(ui, PICO_COLOR_BRIGHT_CYAN, -1);

    /* Line 8: Test Progress */
    pico_ui_clear_line(ui, 8);
    pico_ui_puts_yx(ui, 8, 0, "├─ Test Results:");


    
    /* Line 9: Test Name and millis from the start*/
    for (int i = 0;i < tests_count; ++i)
    {
        taf_state_test_t *test = da_get(taf_state->tests, i);
        pico_ui_clear_line(ui, 9 + i);
        pico_ui_printf_yx(ui, 9 + i, 0, "│  ├─ Name: %s    Result: %s    Started: %s    Finished: %s", test->name, test->status_str, test->started, test->finished);
    }
}

static void render_ui(pico_t *ui, void *ud)
{
    (void)ud;

    pico_remove_cursor();
    
    taf_tui_project_header_render(ui);

    taf_tui_test_progress_render(ui);
    
    taf_tui_summary_render(ui, 12);

}

static void render_progress(pico_t *ui, void *ud) {
    (void)ud;
    
    taf_tui_test_progress_render(ui);
    
    taf_tui_summary_render(ui,12);
}

static void render_result(pico_t *ui, void *ud){
    (void)ud;

    // Numder of tests 
    size_t tests_count = da_size(taf_state->tests); 
    pico_set_ui_rows(ui, tests_count+12);
    
    taf_tui_test_progress_render(ui);
    
    taf_tui_test_run_result(ui, tests_count);

    taf_tui_summary_render(ui, tests_count+9);
}

void taf_tui_test_started(taf_state_test_t *test) {
    
    render_ui(ui, NULL);
    //taf_tui_update();
}

void taf_tui_defer_queue_started(taf_state_test_t *test) {}

void taf_tui_defer_queue_finished(taf_state_test_t *test) {}

void taf_tui_defer_failed(taf_state_test_t *test,
                          taf_state_test_output_t *output) {}

void taf_tui_test_finished(taf_state_test_t *test) { 
    
    //taf_tui_update(); 
}
void taf_tui_tests_set_finished(taf_state_test_t *test)
{
    render_result(ui,NULL);
}

void taf_tui_hooks_started() {}

void taf_tui_hooks_finished() {}

void taf_tui_hook_failed() {}

int taf_tui_init(taf_state_t *state) {

    LOG("Start TUI init");

    taf_state = state;

    //taf_state_register_test_run_started_cb();
    taf_state_register_test_started_cb(state, taf_tui_test_started);
    taf_state_register_test_finished_cb(state, taf_tui_test_finished);
    taf_state_register_test_log_cb(state, taf_tui_log);
    taf_state_register_test_teardown_started_cb(state,
                                                taf_tui_defer_queue_started);
    taf_state_register_test_teardown_finished_cb(state,
                                                 taf_tui_defer_queue_finished);
    taf_state_register_test_defer_failed_cb(state, taf_tui_defer_failed);
    taf_state_register_test_run_finished_cb(state, taf_tui_tests_set_finished);

    // Get project information
    cmd_test_options *opts = cmd_parser_get_test_options();
    ui_state.log_level = opts->log_level;


    // To  write Unicode
    setlocale(LC_ALL, "");

    // UI inintialization
    ui = pico_init(15, render_ui, NULL);
    if (!ui)
        return 1;
    pico_attach(ui);
    pico_install_sigint_handler(ui);
    return 0;
}

void taf_tui_deinit() {

    LOG("Start TUI deinit");
    
    // Turn off UI and free resources
    pico_shutdown(ui);
    
    // Restore cursor 
    pico_restore_cursor();
    
    // Shift to new string
    puts("\n");

    // If tests were launched without log:
    cmd_test_options *opts = cmd_parser_get_test_options();
    if (!opts->no_logs) {
        puts("For more info: 'taf logs info latest'");
    }

    // Freing resources
    pico_free(ui);
}

void taf_tui_update() {

    // UI panel update
    pico_redraw_ui(ui);
}
void taf_tui_set_current_line(const char *file, int line,
                              const char *line_str) {
    free(ui_state.current_file);
    free(ui_state.current_line_str);
    ui_state.current_file = strdup(file);
    ui_state.current_line = line;
    ui_state.current_line_str = string_strip(line_str);
    render_progress(ui, NULL);
    //taf_tui_update();
}

