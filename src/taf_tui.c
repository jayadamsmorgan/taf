#include "taf_tui.h"
#include "util/string.h"

#include <notcurses/notcurses.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static unsigned term_rows = 0, term_cols = 0;

static struct notcurses *nc;
static struct ncplane *tui_plane;

typedef struct {
    int total_tests;

    const char *current_test_str;
    int current_test_index;
    int current_test_progress;

    const char *current_ex_file;
    int current_ex_line;
    const char *current_ex_line_str;

    char *log_lines;

} tui_state_t;

static tui_state_t ui = {
    .total_tests = 0,

    .current_test_str = NULL,
    .current_test_index = 0,
    .current_test_progress = 0,

    .current_ex_file = NULL,
    .current_ex_line = 0,
    .current_ex_line_str = NULL,

    .log_lines = NULL,
};

static inline void tui_log() {

    size_t wrapped;
    size_t *indices =
        string_wrapped_lines(ui.log_lines, ncplane_dim_x(tui_plane), &wrapped);

    ncplane_resize_simple(tui_plane, ncplane_dim_y(tui_plane) + wrapped + 1,
                          ncplane_dim_x(tui_plane));

    ncplane_scrollup_child(notcurses_stdplane(nc), tui_plane);

    for (size_t i = 0; i < wrapped; i++) {
        ncplane_printf_yx(tui_plane,
                          ncplane_dim_y(tui_plane) - wrapped - 11 + i, 0, "%s",
                          &ui.log_lines[indices[i]]);
    }
    free(indices);

    free(ui.log_lines);
    ui.log_lines = NULL;
}

void tui_update() {

    ncplane_erase_region(tui_plane, ncplane_dim_y(tui_plane) - 11, 0, 10,
                         ncplane_dim_x(tui_plane) - 1);
    notcurses_render(nc);

    if (ui.log_lines && *ui.log_lines) {
        tui_log();
    }

    unsigned dimy = ncplane_dim_y(tui_plane);

    ncplane_printf_yx(tui_plane, dimy - 8, 2, "Current test: [%d/%d]: %s",
                      ui.current_test_index, ui.total_tests,
                      ui.current_test_str);

    ncplane_printf_yx(tui_plane, dimy - 7, 2,
                      "Currently executing: [%s at %d]: %s", ui.current_ex_file,
                      ui.current_ex_line, ui.current_ex_line_str);
    ncplane_printf_yx(tui_plane, dimy - 6, 2, "Test progress: %d%%",
                      ui.current_test_progress);

    ncplane_cursor_move_yx(tui_plane, dimy - 10, 0);

    ncplane_rounded_box_sized(tui_plane, 0, 0, 9, ncplane_dim_x(tui_plane), 0);
    ncplane_putstr_yx(tui_plane, dimy - 10, 3, "TAF Test Suite");

    notcurses_render(nc);
}

void taf_tui_set_test_amount(int amount) {
    //
    ui.total_tests = amount;
}

void taf_tui_set_test_progress(int progress) {
    //
    ui.current_test_progress = progress;
}

void taf_tui_set_current_test(int index, const char *test) {
    ui.current_test_str = test;
    ui.current_test_index = index;
}

void taf_tui_set_current_line(const char *file, int line,
                              const char *line_str) {
    ui.current_ex_file = file;
    ui.current_ex_line = line;
    ui.current_ex_line_str = line_str;
}

void taf_tui_test_failed(int index, const char *name, const char *msg) {}

void taf_tui_test_passed(int index, const char *name) {}

void taf_tui_log(const char *file, int line, const char *str) {

    if (ui.log_lines && *ui.log_lines) {
        free(ui.log_lines);
    }

    size_t sz = sizeof(char) * (strlen(file) + strlen(str) + 25);
    ui.log_lines = malloc(sz);

    snprintf(ui.log_lines, sz, "[INFO]: [%s at %d]: %s\n", file, line, str);

    tui_update();
}

int taf_tui_init() {

    notcurses_options ncopts = {
        .flags = NCOPTION_CLI_MODE | NCOPTION_SUPPRESS_BANNERS,
        .loglevel = NCLOGLEVEL_SILENT,
    };
    nc = notcurses_core_init(&ncopts, NULL);
    if (!nc) {
        perror("notcurses_core_init");
        return -1;
    }

    notcurses_term_dim_yx(nc, &term_rows, &term_cols);

    int cury = 0, curx = 0;
    notcurses_cursor_yx(nc, &cury, &curx);

    struct ncplane *std_plane = notcurses_stdplane(nc);

    ncplane_options tui_plane_opts = {
        .y = cury,
        .x = 0,
        .cols = term_cols,
        .rows = 10,
        .name = "tui_plane",
    };

    tui_plane = ncplane_create(std_plane, &tui_plane_opts);
    ncplane_scrollup_child(std_plane, tui_plane);

    tui_update();

    return 0;
}

void taf_tui_deinit(void) {
    notcurses_stop(nc);
    printf("\n");
}
