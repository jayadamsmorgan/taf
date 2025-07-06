#include "taf_tui.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "version.h"

#include "util/string.h"
#include "util/time.h"

#include <notcurses/notcurses.h>

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum {
    PASSED = 0U,
    FAILED = 1U,
    RUNNING = 2U,
} ui_test_progress_state;

typedef struct {
    ui_test_progress_state state;
    unsigned long elapsed;
    char *name;

    char *time;
} ui_test_history_t;

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

static ui_state_t ui = {0};

static struct notcurses *nc = NULL;
static struct ncplane *log_plane = NULL;
static struct ncplane *ui_plane = NULL;
static struct ncplane *test_progress_plane = NULL;
static struct ncprogbar *test_progress_bar_current = NULL;
static struct ncprogbar *test_progress_bar_total = NULL;

static uint absy = 0, absx = 0;

static uint project_info_dimy = 2;

static inline void print_hline(struct ncplane *plane, int y) {
    int dx = ncplane_dim_x(plane);
    ncplane_putstr_yx(plane, y, 0, "├");
    ncplane_putstr_yx(plane, y, dx - 1, "┤");
    for (int i = 1; i < dx - 1; i++) {
        ncplane_putstr_yx(plane, y, i, "─");
    }
}

static int log_level_to_palindex_map[] = {
    9, 1, 3, 4, 2, 6,
};

static int test_state_to_palindex_map[] = {
    2,
    1,
    5,
};

static const char *test_state_to_str_map[] = {
    "PASSED",
    "FAILED",
    "RUNNING",
};

void taf_tui_update() {

    ncplane_dim_yx(notcurses_stdplane(nc), &absy, &absx);
    ncplane_erase(notcurses_stdplane(nc));
    ncplane_erase(ui_plane);
    ncplane_perimeter_rounded(ui_plane, 0, 0, 0);
    ncplane_putstr_aligned(ui_plane, 0, NCALIGN_CENTER,
                           " TAF v" TAF_VERSION " ");
    print_hline(ui_plane, project_info_dimy + 1);
    ncplane_putstr_aligned(ui_plane, project_info_dimy + 1, NCALIGN_CENTER,
                           " Test Progress ");
    print_hline(ui_plane, project_info_dimy + 10);
    ncplane_putstr_aligned(ui_plane, project_info_dimy + 10, NCALIGN_CENTER,
                           " Summary ");

    ncplane_printf_yx(ui_plane, project_info_dimy + 11, 2, "Total: %d",
                      ui.total_tests);
    ncplane_putstr(ui_plane, " | ");

    ncplane_set_fg_palindex(ui_plane, 2); // Green
    ncplane_printf(ui_plane, "Passed: %d", ui.passed_tests);

    ncplane_set_fg_default(ui_plane);
    ncplane_putstr(ui_plane, " | ");

    ncplane_set_fg_palindex(ui_plane, 1); // Red
    ncplane_printf(ui_plane, "Failed: %d", ui.failed_tests);

    ncplane_set_fg_default(ui_plane);

    uint64_t ms;
    if (ui.passed_tests + ui.failed_tests == ui.total_tests) {
        // Probably finished executing
        ms = ui.total_elapsed_ms;
    } else {
        ms = millis_since_taf_start();
    }
    const unsigned long minutes = ms / 60000;
    const unsigned long seconds = (ms / 1000) % 60;
    const unsigned long millis = ms % 1000;
    ncplane_printf_aligned(ui_plane, project_info_dimy + 11, NCALIGN_RIGHT,
                           "Elapsed Time: %lum %lu.%03lus │", minutes, seconds,
                           millis);
    ncplane_printf_yx(ui_plane, 1, 2, "Project: %s", ui.project_name);
    int offset = 0;
    if (ui.target) {
        ncplane_printf_yx(ui_plane, 2, 3, "Target: %s", ui.target);
        offset++;
    }
    if (ui.tags) {
        ncplane_printf_yx(ui_plane, 2 + offset, 5, "Tags: %s", ui.tags);
        offset++;
    }
    ncplane_putstr_yx(ui_plane, 2 + offset, 6, "Log: ");
    ncplane_set_fg_palindex(ui_plane, log_level_to_palindex_map[ui.log_level]);
    ncplane_putstr(ui_plane, taf_log_level_to_str(ui.log_level));
    ncplane_set_fg_default(ui_plane);
    if (ui.no_logs) {
        ncplane_putstr(ui_plane, " (No File Logs)");
    }

    ncplane_erase(test_progress_plane);
    offset = 0;
    for (int i = ui.test_history_size - 1; i >= 0; i--) {
        if (offset >= 4) {
            break;
        }
        ui_test_history_t *hist = &ui.test_history[i];
        ncplane_set_fg_palindex(test_progress_plane,
                                test_state_to_palindex_map[hist->state]);
        ncplane_putstr_yx(test_progress_plane, 3 - offset, 2,
                          test_state_to_str_map[hist->state]);
        ncplane_set_fg_default(test_progress_plane);
        ncplane_printf_yx(test_progress_plane, 3 - offset, 10, "[ %lums ] %s",
                          hist->state == RUNNING ? millis_since_start()
                                                 : hist->elapsed,
                          hist->name);
        if (hist->state == RUNNING) {
            char *file_str = ui.current_file;
            size_t len = strlen(ui.current_file);
            if (len > 40) {
                file_str += len - 40;
            }
            ncplane_printf(test_progress_plane, "...    [%s%s:%d]",
                           len > 40 ? "..." : "", file_str, ui.current_line);
        }
        offset++;
    }

    double total_progress =
        (double)(ui.passed_tests + ui.failed_tests) / ui.total_tests;
    if (ui.current_test_index != ui.total_tests) {
        total_progress += ui.current_test_progress / ui.total_tests;
    }
    ncprogbar_set_progress(test_progress_bar_current, ui.current_test_progress);
    ncprogbar_set_progress(test_progress_bar_total, total_progress);
    ncplane_putstr_yx(ui_plane, project_info_dimy + 8, 2, "Current:");
    ncplane_putstr_yx(ui_plane, project_info_dimy + 9, 4, "Total:");
    ncplane_printf_yx(ui_plane, project_info_dimy + 8, absx - 8, "%.2f%%",
                      ui.current_test_progress * 100);
    ncplane_printf_yx(ui_plane, project_info_dimy + 9, absx - 8, "%.2f%%",
                      total_progress * 100);

    notcurses_render(nc);
}

void taf_tui_set_test_amount(int amount) {
    //
    ui.total_tests = amount;
}

void taf_tui_set_test_progress(double progress) {
    //
    ui.current_test_progress = progress;
}

void taf_tui_set_current_test(int index, const char *test) {
    ui.current_test_index = index;
    ui.test_history_size++;
    if (ui.test_history_size >= ui.test_history_cap) {
        ui.test_history_cap *= 2;
        ui.test_history = realloc(ui.test_history, sizeof(*ui.test_history) *
                                                       ui.test_history_cap);
    }
    ui_test_history_t *hist = &ui.test_history[ui.test_history_size - 1];
    hist->name = strdup(test);
    hist->state = RUNNING;
    hist->elapsed = 0;

    ncplane_move_rel(ui_plane, 2, 0);
    ncplane_erase(ui_plane);
    notcurses_render(nc);
    ncplane_scrollup_child(notcurses_stdplane(nc), ui_plane);
    ncplane_resize_simple(log_plane, ncplane_dim_y(log_plane) + 2, absx);
    char ts[TS_LEN];
    get_date_time_now(ts);
    ncplane_printf_yx(log_plane, ncplane_dim_y(log_plane) - 2, 0, "%s ", ts);
    ncplane_set_fg_palindex(log_plane, 5);
    ncplane_printf(log_plane, "Test '%s' STARTED...", hist->name);
    ncplane_set_fg_default(log_plane);
    for (uint i = 0; i < ncplane_dim_x(log_plane); i++) {
        ncplane_putstr_yx(log_plane, ncplane_dim_y(log_plane) - 1, i, "─");
    }
}

void taf_tui_set_current_line(const char *file, int line,
                              const char *line_str) {
    free(ui.current_file);
    free(ui.current_line_str);
    ui.current_file = strdup(file);
    ui.current_line = line;
    ui.current_line_str = string_strip(line_str);
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
    if (log_level > ui.log_level) {
        return;
    }
    // This is a total mess...
    // I don't like it even one bit but it works for now...
    char *tmp = malloc(buffer_len + 1);
    memcpy(tmp, buffer, buffer_len);
    tmp[buffer_len] = '\0';
    sanitize_inplace(tmp, buffer_len);
    size_t count;
    size_t *indices = string_wrapped_lines(tmp, absx, &count);
    ncplane_move_rel(ui_plane, 2, 0);
    ncplane_erase(ui_plane);
    notcurses_render(nc);
    ncplane_scrollup_child(notcurses_stdplane(nc), ui_plane);
    ncplane_resize_simple(log_plane, ncplane_dim_y(log_plane) + 2, absx);
    ncplane_printf_yx(log_plane, ncplane_dim_y(log_plane) - 2, 0, "%s ", time);
    ncplane_set_fg_palindex(log_plane, log_level_to_palindex_map[log_level]);
    ncplane_printf(log_plane, "[%s]", taf_log_level_to_str(log_level));
    ncplane_set_fg_default(log_plane);
    ncplane_putstr(log_plane, ":");
    for (size_t i = 0; i < count; i++) {
        ncplane_move_rel(ui_plane, 1, 0);
        ncplane_erase(ui_plane);
        notcurses_render(nc);
        ncplane_scrollup_child(notcurses_stdplane(nc), ui_plane);
        ncplane_putstr_aligned(log_plane, ncplane_dim_y(log_plane) - 1,
                               NCALIGN_LEFT, &tmp[indices[i]]);
        ncplane_resize_simple(log_plane, ncplane_dim_y(log_plane) + 1, absx);
    }
    free(indices);
    free(tmp);
    for (uint i = 0; i < ncplane_dim_x(log_plane); i++) {
        ncplane_putstr_yx(log_plane, ncplane_dim_y(log_plane) - 1, i, "─");
    }
    taf_tui_update();
}

void taf_tui_defer_queue_started(char *time) {
    ui_test_history_t *hist = &ui.test_history[ui.test_history_size - 1];

    ncplane_move_rel(ui_plane, 2, 0);
    ncplane_erase(ui_plane);
    notcurses_render(nc);
    ncplane_scrollup_child(notcurses_stdplane(nc), ui_plane);
    ncplane_resize_simple(log_plane, ncplane_dim_y(log_plane) + 2, absx);
    ncplane_printf_yx(log_plane, ncplane_dim_y(log_plane) - 2, 0, "%s ", time);
    ncplane_set_fg_palindex(log_plane, 5);
    ncplane_printf(log_plane, "Defer Queue for Test '%s' STARTED...",
                   hist->name);
    ncplane_set_fg_default(log_plane);
    for (uint i = 0; i < ncplane_dim_x(log_plane); i++) {
        ncplane_putstr_yx(log_plane, ncplane_dim_y(log_plane) - 1, i, "─");
    }
}

void taf_tui_defer_queue_finished(char *time) {
    ui_test_history_t *hist = &ui.test_history[ui.test_history_size - 1];

    ncplane_move_rel(ui_plane, 2, 0);
    ncplane_erase(ui_plane);
    notcurses_render(nc);
    ncplane_scrollup_child(notcurses_stdplane(nc), ui_plane);
    ncplane_resize_simple(log_plane, ncplane_dim_y(log_plane) + 2, absx);
    ncplane_printf_yx(log_plane, ncplane_dim_y(log_plane) - 2, 0, "%s ", time);
    ncplane_set_fg_palindex(log_plane, 2);
    ncplane_printf(log_plane, "Defer Queue for Test '%s' FINISHED...",
                   hist->name);
    ncplane_set_fg_default(log_plane);
    for (uint i = 0; i < ncplane_dim_x(log_plane); i++) {
        ncplane_putstr_yx(log_plane, ncplane_dim_y(log_plane) - 1, i, "─");
    }
}

void taf_tui_test_passed(char *time) {
    ui_test_history_t *hist = &ui.test_history[ui.test_history_size - 1];
    hist->state = PASSED;
    hist->elapsed = millis_since_start();
    hist->time = strdup(time);
    ui.passed_tests++;

    ncplane_move_rel(ui_plane, 2, 0);
    ncplane_erase(ui_plane);
    notcurses_render(nc);
    ncplane_scrollup_child(notcurses_stdplane(nc), ui_plane);
    ncplane_resize_simple(log_plane, ncplane_dim_y(log_plane) + 2, absx);
    ncplane_printf_yx(log_plane, ncplane_dim_y(log_plane) - 2, 0, "%s ", time);
    ncplane_set_fg_palindex(log_plane, 2);
    ncplane_printf(log_plane, "Test '%s' PASSED", hist->name);
    ncplane_set_fg_default(log_plane);
    for (uint i = 0; i < ncplane_dim_x(log_plane); i++) {
        ncplane_putstr_yx(log_plane, ncplane_dim_y(log_plane) - 1, i, "─");
    }

    if (ui.passed_tests + ui.failed_tests == ui.total_tests) {
        ui.total_elapsed_ms = millis_since_taf_start();
    }
}

void taf_tui_test_failed(char *time, raw_log_test_output_t *failure_reasons,
                         size_t failure_reasons_count) {
    ui_test_history_t *hist = &ui.test_history[ui.test_history_size - 1];
    hist->state = FAILED;
    hist->elapsed = millis_since_start();
    hist->time = strdup(time);
    ui.failed_tests++;

    ncplane_move_rel(ui_plane, 2, 0);
    ncplane_erase(ui_plane);
    notcurses_render(nc);
    ncplane_scrollup_child(notcurses_stdplane(nc), ui_plane);
    ncplane_resize_simple(log_plane, ncplane_dim_y(log_plane) + 2, absx);
    ncplane_printf_yx(log_plane, ncplane_dim_y(log_plane) - 2, 0, "%s ", time);
    ncplane_set_fg_palindex(log_plane, 1);
    ncplane_printf(log_plane, "Test '%s' FAILED:", hist->name);

    for (size_t j = 0; j < failure_reasons_count; j++) {
        ncplane_move_rel(ui_plane, 1, 0);
        ncplane_erase(ui_plane);
        notcurses_render(nc);
        ncplane_scrollup_child(notcurses_stdplane(nc), ui_plane);
        ncplane_resize_simple(log_plane, ncplane_dim_y(log_plane) + 1, absx);

        char *tmp = strdup(failure_reasons[j].msg);
        sanitize_inplace(tmp, failure_reasons[j].msg_len);

        ncplane_move_rel(ui_plane, 1, 0);
        ncplane_erase(ui_plane);
        notcurses_render(nc);
        ncplane_scrollup_child(notcurses_stdplane(nc), ui_plane);
        ncplane_set_fg_default(log_plane);
        ncplane_printf_yx(log_plane, ncplane_dim_y(log_plane) - 1, 0,
                          "Failure reason %zu: [", j + 1);
        ncplane_set_fg_palindex(log_plane, 1);
        ncplane_printf(log_plane, "%s",
                       taf_log_level_to_str(failure_reasons[j].level));
        ncplane_set_fg_default(log_plane);
        ncplane_putstr(log_plane, "]:");
        ncplane_resize_simple(log_plane, ncplane_dim_y(log_plane) + 1, absx);

        ncplane_set_fg_palindex(log_plane, 1);

        size_t count;
        size_t *indices = string_wrapped_lines(tmp, absx, &count);
        for (size_t i = 0; i < count; i++) {
            ncplane_move_rel(ui_plane, 1, 0);
            ncplane_erase(ui_plane);
            notcurses_render(nc);
            ncplane_scrollup_child(notcurses_stdplane(nc), ui_plane);
            ncplane_putstr_aligned(log_plane, ncplane_dim_y(log_plane) - 1,
                                   NCALIGN_LEFT, &tmp[indices[i]]);
            ncplane_resize_simple(log_plane, ncplane_dim_y(log_plane) + 1,
                                  absx);
        }
        free(indices);
        free(tmp);
    }

    ncplane_set_fg_default(log_plane);
    for (uint i = 0; i < ncplane_dim_x(log_plane); i++) {
        ncplane_putstr_yx(log_plane, ncplane_dim_y(log_plane) - 1, i, "─");
    }

    if (ui.passed_tests + ui.failed_tests == ui.total_tests) {
        ui.total_elapsed_ms = millis_since_taf_start();
    }
}

static void init_project_info() {
    cmd_test_options *opts = cmd_parser_get_test_options();
    project_parsed_t *proj = get_parsed_project();
    ui.project_name = strdup(proj->project_name);
    if (proj->multitarget && opts->target) {
        ui.target = strdup(opts->target);
        project_info_dimy++;
    }
    if (opts->tags_amount != 0) {
        ui.tags = string_join(opts->tags, opts->tags_amount);
        project_info_dimy++;
    }
    ui.log_level = opts->log_level;
    ui.no_logs = opts->no_logs;
}

static int stdplane_resize_cb(struct ncplane *) {
    notcurses_refresh(nc, NULL, NULL);
    taf_tui_update();
    return 0;
}

static int progress_bar_resize_cb(struct ncplane *plane) {
    ncplane_dim_yx(notcurses_stdplane(nc), &absy, &absx);
    ncplane_resize_simple(plane, 1, absx - 18);
    taf_tui_update();
    return 0;
}

static int log_plane_resize_cb(struct ncplane *) {
    // ncplane_dim_yx(notcurses_stdplane(nc), &absy, &absx);
    // ncplane_resize_simple(plane, absy - 13 - project_info_dimy, absx - 2);
    return 0;
}

static int ui_plane_resize_cb(struct ncplane *plane) {
    ncplane_dim_yx(notcurses_stdplane(nc), &absy, &absx);
    ncplane_resize_simple(plane, 13 + project_info_dimy, absx);
    taf_tui_update();
    return 0;
}

static int test_progress_plane_resize_cb(struct ncplane *plane) {
    uint uiy, uix;
    ncplane_dim_yx(ui_plane, &uiy, &uix);
    ncplane_resize_simple(plane, 4, uix - 6);
    taf_tui_update();
    return 0;
}

int taf_tui_init() {

    init_project_info();

    ui.test_history_cap = 10;
    ui.test_history = calloc(ui.test_history_cap, sizeof(*ui.test_history));

    setlocale(LC_ALL, "");
    notcurses_options opts = {
        .flags =
            NCOPTION_CLI_MODE | NCOPTION_SCROLLING | NCOPTION_SUPPRESS_BANNERS,
        .loglevel = NCLOGLEVEL_SILENT,
    };
    nc = notcurses_core_init(&opts, NULL);
    if (!nc) {
        fprintf(stderr, "Unable to init notcurses library.\n");
        return -1;
    }

    struct ncplane *stdplane = notcurses_stdplane(nc);

    ncplane_dim_yx(stdplane, &absy, &absx);
    if (absy < 20 || absx < 85) {
        fprintf(stderr,
                "Unable to init TAF TUI: Terminal window size has to "
                "be at least 85x20. Current size: %dx%d.\n",
                absx, absy);
        return -1;
    }

    ncplane_set_resizecb(stdplane, stdplane_resize_cb);

    ncplane_options ui_plane_opts = {
        .x = 0,
        .y = absy - 1,
        .cols = absx,
        .rows = 13 + project_info_dimy,
        .resizecb = ui_plane_resize_cb,
    };
    ui_plane = ncplane_create(stdplane, &ui_plane_opts);
    ncplane_scrollup_child(stdplane, ui_plane);
    ncplane_move_rel(ui_plane, 1, 0);
    ncplane_scrollup_child(stdplane, ui_plane);

    uint uiy, uix;
    ncplane_dim_yx(ui_plane, &uiy, &uix);

    ncplane_options test_progress_plane_opts = {
        .x = 2,
        .y = uiy - 10,
        .cols = uix - 4,
        .rows = 4,
        .resizecb = test_progress_plane_resize_cb,
    };
    test_progress_plane = ncplane_create(ui_plane, &test_progress_plane_opts);

    ncplane_options test_progress_bar_opts = {
        .x = 11,
        .y = uiy - 5,
        .cols = uix - 20,
        .rows = 1,
        .resizecb = progress_bar_resize_cb,
    };
    struct ncplane *test_progress_bar_current_plane =
        ncplane_create(ui_plane, &test_progress_bar_opts);
    test_progress_bar_opts.y = uiy - 4;
    struct ncplane *test_progress_bar_total_plane =
        ncplane_create(ui_plane, &test_progress_bar_opts);
    ncprogbar_options progbar_opts = {0};
    test_progress_bar_current =
        ncprogbar_create(test_progress_bar_current_plane, &progbar_opts);
    test_progress_bar_total =
        ncprogbar_create(test_progress_bar_total_plane, &progbar_opts);

    ncplane_options log_plane_opts = {
        .x = 0,
        .y = ncplane_abs_y(ui_plane) - 1,
        .cols = absx,
        .rows = 1,
        .resizecb = log_plane_resize_cb,
    };
    log_plane = ncplane_create(stdplane, &log_plane_opts);

    taf_tui_update();

    return 0;
}

void taf_tui_deinit() {

    taf_tui_update();

    notcurses_stop(nc);

    puts("\n");
    cmd_test_options *opts = cmd_parser_get_test_options();
    if (!opts->no_logs) {
        puts("For more info: 'taf logs info latest'");
    }

    for (size_t i = 0; i < ui.test_history_size; i++) {
        ui_test_history_t *hist = &ui.test_history[i];
        free(hist->name);
        free(hist->time);
    }
    free(ui.test_history);

    free(ui.current_line_str);
    free(ui.current_file);
    free(ui.project_name);
    free(ui.tags);
    free(ui.target);
}
