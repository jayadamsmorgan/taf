#include "taf_tui.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "version.h"

#include "internal_logging.h"
#include "util/string.h"
#include "util/time.h"

#include <picotui.h>

#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Possible test result
typedef enum {
    PASSED = 0U,
    FAILED = 1U,
    RUNNING = 2U,
} ui_test_progress_state;

static const char *test_state_to_str_map[] = {
    "PASSED",
    "FAILED",
    "RUNNING",
};

// 
typedef struct {
    ui_test_progress_state state;
    unsigned long elapsed;
    char *name;

    char *time;
} ui_test_history_t;

// 
typedef struct {
    char *project_name;
    char *target;
    char *tags;

    taf_log_level log_level;
    bool no_logs;

    ui_test_history_t *test_history;
    size_t test_history_size;
    size_t test_history_cap;

    int total_tests;
    int current_test_index;
    int passed_tests;
    int failed_tests;

    double current_test_progress;
    char *current_file;
    int current_line;
    char *current_line_str;

    uint64_t total_elapsed_ms;
} ui_state_t;


static ui_state_t ui_state = {0};

static pico_t *ui = NULL;








//

static uint absy = 0, absx = 0;

static uint project_info_dimy = 2;

static int log_level_to_palindex_map[] = {
    9, 1, 3, 4, 2, 6,
};

static int test_state_to_palindex_map[] = {
    2,
    1,
    5,
};



/*------------------- Helper functions -------------------*/

static void init_project_info() {
    cmd_test_options *opts = cmd_parser_get_test_options();
    project_parsed_t *proj = get_parsed_project();
    ui_state.project_name = strdup(proj->project_name);

    if (proj->multitarget && opts->target) {
        ui_state.target = strdup(opts->target);
        project_info_dimy++;
    }
    if (opts->tags_amount != 0) {
        ui_state.tags = string_join(opts->tags, opts->tags_amount);
        project_info_dimy++;
    }
    ui_state.log_level = opts->log_level;
    ui_state.no_logs = opts->no_logs;
}

void taf_tui_set_test_amount(int amount) {
    //
    ui_state.total_tests = amount;
}

void taf_tui_set_test_progress(double progress) {
    //
    ui_state.current_test_progress = progress;
}

void taf_tui_set_current_line(const char *file, int line,
                              const char *line_str) {
    free(ui_state.current_file);
    free(ui_state.current_line_str);
    ui_state.current_file = strdup(file);
    ui_state.current_line = line;
    ui_state.current_line_str = string_strip(line_str);
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

void taf_tui_log(char *time, taf_log_level log_level, const char *, int,
                 const char *buffer, size_t buffer_len) {
    
    // Filter for logs with lower log level 
    if (log_level > ui_state.log_level) {
        return;
    }
    
    // Allocate space for logs  and remove inapropriate synbols from it
    char *tmp = malloc(buffer_len + 1);
    memcpy(tmp, buffer, buffer_len);
    tmp[buffer_len] = '\0';
    sanitize_inplace(tmp, buffer_len);
   
    // Write logs header 
    pico_set_colors(ui, PICO_COLOR_WHITE, -1);
    pico_printf(ui, "%s:", time);
    pico_set_colors(ui, log_level_to_palindex_map[log_level], -1);
    pico_printf(ui, "[%s]", taf_log_level_to_str(log_level));
    pico_set_colors(ui, log_level_to_palindex_map[log_level], -1);
    pico_printf(ui, ":");

    // Write logs body 
    pico_set_colors(ui, PICO_COLOR_WHITE, -1);
    pico_print_block(ui, tmp);
    free(tmp);

    taf_tui_update();
}

void taf_tui_set_current_test(int index, const char *test) {
    ui_state.current_test_index = index;
    ui_state.test_history_size++;
    if (ui_state.test_history_size >= ui_state.test_history_cap) {
        ui_state.test_history_cap *= 2;
        ui_state.test_history = realloc(ui_state.test_history, sizeof(*ui_state.test_history) *
                                                       ui_state.test_history_cap);
    }
    ui_test_history_t *hist = &ui_state.test_history[ui_state.test_history_size - 1];
    hist->name = strdup(test);
    hist->state = RUNNING;
    hist->elapsed = 0;
}

void taf_tui_defer_queue_started(char *time) {
}

void taf_tui_defer_queue_finished(char *time) {
}

void taf_tui_test_passed(char *time) {
}

void taf_tui_defer_failed(char *time, const char *trace, const char *file,
                          int line) {
}

void taf_tui_test_failed(char *time, raw_log_test_output_t *failure_reasons,
                         size_t failure_reasons_count) {
}

void taf_tui_hooks_started(char *time) {
}

void taf_tui_hooks_finished(char *time) {
}

void taf_tui_hook_failed(char *time, const char *trace) {
}
/*------------------- TAF UI functions -------------------*/
static void render_ui(pico_t *ui, void *ud) {
    (void)ud;

    pico_reset_colors(ui); // better to do it

    /* Line 0: Main title */
    pico_ui_clear_line(ui, 0);
    pico_ui_puts_yx(ui, 0, 0, "TAF v" TAF_VERSION " ");

    /* Line 1: Project name */
    pico_ui_clear_line(ui, 1);
    pico_ui_printf_yx(ui, 1, 0,  "Project: %s",ui_state.project_name);
    
    /* Line 2: Target */
    pico_ui_clear_line(ui, 2);
    pico_ui_printf_yx(ui, 2, 0,   "Target: %s", ui_state.target);
    
    /* Line 3: Log level */
    pico_ui_clear_line(ui, 3);
    pico_ui_printf_yx(ui, 3, 0,   "Log: ");
    pico_set_colors(ui, log_level_to_palindex_map[ui_state.log_level], -1);
    pico_ui_printf_yx(ui, 3, 5, "%s",taf_log_level_to_str(ui_state.log_level));

    /* Line 4: Test Progress Title */
    pico_set_colors(ui, PICO_COLOR_WHITE, -1);
    pico_ui_clear_line(ui, 4);
    pico_ui_printf_yx(ui, 4, 0,   "Test Progress");

}

int taf_tui_init() {

    LOG("Start TUI init");
    // Get project information 
    init_project_info();
    
    ui_state.test_history_cap = 10;
    ui_state.test_history = calloc(ui_state.test_history_cap, sizeof(*ui_state.test_history));



    // To  write Unicode
    setlocale(LC_ALL, "");
 
    // UI inintialization
    ui = pico_init(5, render_ui, NULL);
    if (!ui) return 1;
    
    pico_attach(ui);
    pico_install_sigint_handler(ui);
    // Creste standart plane
    
    // Check terminal size
    
    // Panel resize

    // Progress bar plane init

    // Update UI

    return 0;
}

void taf_tui_deinit() {

    LOG("Start TUI deinit");
    
    // Update UI last time
    taf_tui_update();
    
    // Turn off UI and free resources
    pico_shutdown(ui);
    
    // Shift to new string
    puts("\n");
    
    // If tests were launched without log:
    cmd_test_options *opts = cmd_parser_get_test_options();
    if (!opts->no_logs) {
        puts("For more info: 'taf logs info latest'");
    }

    // Freing resources
    for (size_t i = 0; i < ui_state.test_history_size; i++) {
        ui_test_history_t *hist = &ui_state.test_history[i];
        free(hist->name);
        free(hist->time);
    }
    
    pico_free(ui);

    free(ui_state.test_history);
    free(ui_state.current_line_str);
    free(ui_state.current_file);
    free(ui_state.project_name);
    free(ui_state.tags);
    free(ui_state.target);
}


void taf_tui_update() {
    

    // UI panel update
    pico_redraw_ui(ui); 
    
}






