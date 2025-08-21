#include "picotui.h"

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unibilium.h>
#include <unistd.h>

typedef struct {
    int rows, cols;
} term_size_t;

struct pico_t {
    int ui_rows;
    term_size_t sz;
    unibi_term *ut;

    /* terminfo caps (may be NULL -> ANSI fallback) */
    const char *cap_cup;   /* cursor_address */
    const char *cap_csr;   /* change_scroll_region */
    const char *cap_el;    /* clr_eol */
    const char *cap_sgr0;  /* exit_attribute_mode (reset) */
    const char *cap_setaf; /* set_a_foreground (16/256 palette) */
    const char *cap_setab; /* set_a_background (16/256 palette) */

    pico_render_fn render;
    void *render_ud;

    volatile sig_atomic_t resized;
};

static pico_t *g_ui_singleton = NULL;

/* ---------- low-level write ---------- */
static void write_str(const char *s, size_t n) {
    while (n) {
        ssize_t w = write(STDOUT_FILENO, s, n);
        if (w > 0) {
            s += w;
            n -= (size_t)w;
        } else if (w < 0 && errno == EINTR)
            continue;
        else
            break;
    }
}
static void write_cstr(const char *s) { write_str(s, strlen(s)); }

static void write_vprintf(const char *prefix, const char *fmt, va_list ap) {
    char stack[1024];
    int n = vsnprintf(stack, sizeof(stack), fmt, ap);
    if (prefix)
        write_cstr(prefix);
    if (n >= 0 && (size_t)n < sizeof(stack)) {
        write_str(stack, (size_t)n);
        return;
    }
    size_t need = (n > 0 ? (size_t)n + 1 : 4096);
    char *buf = malloc(need);
    if (!buf)
        return;
    va_list ap2;
    va_copy(ap2, ap);
    vsnprintf(buf, need, fmt, ap2);
    va_end(ap2);
    write_cstr(buf);
    free(buf);
}

/* ---------- size / signals ---------- */
static term_size_t get_term_size(void) {
    struct winsize ws;
    term_size_t r = {.rows = 24, .cols = 80};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row && ws.ws_col) {
        r.rows = ws.ws_row;
        r.cols = ws.ws_col;
    }
    return r;
}
static void on_sigwinch(int sig) {
    (void)sig;
    if (g_ui_singleton)
        g_ui_singleton->resized = 1;
}

/* ---------- unibilium helpers ---------- */
static void emit_unibi_fmt2(const char *fmt, int p1, int p2) {
    unibi_var_t params[9] = {0};
    params[0] = unibi_var_from_num(p1);
    params[1] = unibi_var_from_num(p2);
    char buf[128];
    size_t need = unibi_run(fmt, params, buf, sizeof(buf));
    if (need <= sizeof(buf))
        write_str(buf, need);
    else {
        char *tmp = malloc(need);
        if (!tmp)
            return;
        unibi_run(fmt, params, tmp, need);
        write_str(tmp, need);
        free(tmp);
    }
}
static void emit_unibi_fmt1(const char *fmt, int p1) {
    unibi_var_t params[9] = {0};
    params[0] = unibi_var_from_num(p1);
    char buf[128];
    size_t need = unibi_run(fmt, params, buf, sizeof(buf));
    if (need <= sizeof(buf))
        write_str(buf, need);
    else {
        char *tmp = malloc(need);
        if (!tmp)
            return;
        unibi_run(fmt, params, tmp, need);
        write_str(tmp, need);
        free(tmp);
    }
}

static void emit_cup(pico_t *ui, int row0, int col0) {
    if (ui->cap_cup) {
        emit_unibi_fmt2(ui->cap_cup, row0, col0);
        return;
    }
    char b[64];
    int r = row0 + 1, c = col0 + 1;
    int n = snprintf(b, sizeof(b), "\x1b[%d;%dH", r, c);
    write_str(b, (size_t)n);
}
static void emit_csr(pico_t *ui, int top0, int bot0) {
    if (ui->cap_csr) {
        emit_unibi_fmt2(ui->cap_csr, top0, bot0);
        return;
    }
    char b[64];
    int t = top0 + 1, btm = bot0 + 1;
    int n = snprintf(b, sizeof(b), "\x1b[%d;%dr", t, btm);
    write_str(b, (size_t)n);
}
static void emit_el(pico_t *ui) {
    if (ui->cap_el) {
        write_cstr(ui->cap_el);
        return;
    }
    write_cstr("\x1b[K");
}

/* ---------- colors (16-color fg/bg) ---------- */
static int clamp16(int v) { return v < 0 ? -1 : (v > 15 ? 15 : v); }

int pico_set_colors(pico_t *ui, int fg16, int bg16) {
    fg16 = clamp16(fg16);
    bg16 = clamp16(bg16);

    /* Try terminfo setaf/setab (expects color index) */
    if (fg16 >= 0) {
        if (ui->cap_setaf)
            emit_unibi_fmt1(ui->cap_setaf, fg16);
        else {
            /* ANSI fallback: map 0..7 -> 30..37, 8..15 -> 90..97 */
            int code = (fg16 < 8 ? 30 + fg16 : 90 + (fg16 - 8));
            char b[16];
            int n = snprintf(b, sizeof(b), "\x1b[%dm", code);
            write_str(b, (size_t)n);
        }
    }
    if (bg16 >= 0) {
        if (ui->cap_setab)
            emit_unibi_fmt1(ui->cap_setab, bg16);
        else {
            /* ANSI fallback: map 0..7 -> 40..47, 8..15 -> 100..107 */
            int code = (bg16 < 8 ? 40 + bg16 : 100 + (bg16 - 8));
            char b[16];
            int n = snprintf(b, sizeof(b), "\x1b[%dm", code);
            write_str(b, (size_t)n);
        }
    }
    return 0;
}

void pico_reset_colors(pico_t *ui) {
    if (ui->cap_sgr0)
        write_cstr(ui->cap_sgr0);
    else
        write_cstr("\x1b[0m");
}

/* ---------- core ops ---------- */
static void set_scroll_region(pico_t *ui, int top0, int bot0) {
    emit_csr(ui, top0, bot0);
}
static void reset_scroll_region(pico_t *ui) {
    set_scroll_region(ui, 0, ui->sz.rows - 1);
}

static void move_to_bottom_scroll_line(pico_t *ui) {
    int row = ui->sz.rows - ui->ui_rows - 1;
    if (row < 0)
        row = 0;
    emit_cup(ui, row, 0);
}

static void clear_ui_region(pico_t *ui) {
    int start = ui->sz.rows - ui->ui_rows;
    if (start < 0)
        start = 0;
    for (int i = 0; i < ui->ui_rows; ++i) {
        emit_cup(ui, start + i, 0);
        emit_el(ui);
    }
}

void pico_ui_puts(pico_t *ui, int rel_row, int col, const char *s) {
    int base = ui->sz.rows - ui->ui_rows;
    if (base < 0)
        base = 0;
    int r = base + (rel_row < 0 ? 0 : rel_row);
    if (r >= ui->sz.rows)
        return;
    if (col < 0)
        col = 0;
    emit_cup(ui, r, col);
    write_cstr(s);
}

void pico_ui_printf(pico_t *ui, int rel_row, int col, const char *fmt, ...) {
    int base = ui->sz.rows - ui->ui_rows;
    if (base < 0)
        base = 0;
    int r = base + (rel_row < 0 ? 0 : rel_row);
    if (r >= ui->sz.rows)
        return;
    if (col < 0)
        col = 0;
    emit_cup(ui, r, col);
    va_list ap;
    va_start(ap, fmt);
    write_vprintf(NULL, fmt, ap);
    va_end(ap);
}

void pico_ui_clear_line(pico_t *ui, int rel_row) {
    int base = ui->sz.rows - ui->ui_rows;
    if (base < 0)
        base = 0;
    int r = base + (rel_row < 0 ? 0 : rel_row);
    if (r >= ui->sz.rows)
        return;
    emit_cup(ui, r, 0);
    emit_el(ui);
}

void pico_redraw_ui(pico_t *ui) {
    clear_ui_region(ui);
    if (ui->render)
        ui->render(ui, ui->render_ud);
}

static void reprogram_region_and_redraw(pico_t *ui) {
    /* Give control back, re-query size, clamp ui_rows, re-apply region, redraw.
     */
    reset_scroll_region(ui);
    ui->sz = get_term_size();
    if (ui->ui_rows < 1)
        ui->ui_rows = 1;
    if (ui->ui_rows >= ui->sz.rows)
        ui->ui_rows = ui->sz.rows - 1;
    int bot = ui->sz.rows - ui->ui_rows - 1;
    if (bot < 0)
        bot = 0;
    set_scroll_region(ui, 0, bot);
    pico_redraw_ui(ui);
    move_to_bottom_scroll_line(ui);
}

static void handle_resize_if_needed(pico_t *ui) {
    if (!ui->resized)
        return;
    ui->resized = 0;
    reprogram_region_and_redraw(ui);
}

pico_t *pico_init(int ui_rows, pico_render_fn render, void *userdata) {
    pico_t *ui = calloc(1, sizeof(*ui));
    if (!ui)
        return NULL;
    ui->ui_rows = (ui_rows < 1 ? 1 : ui_rows);
    ui->render = render;
    ui->render_ud = userdata;

    ui->ut = unibi_from_env();
    if (ui->ut) {
        ui->cap_cup = unibi_get_str(ui->ut, unibi_cursor_address);
        ui->cap_csr = unibi_get_str(ui->ut, unibi_change_scroll_region);
        ui->cap_el = unibi_get_str(ui->ut, unibi_clr_eol);
        ui->cap_sgr0 = unibi_get_str(ui->ut, unibi_exit_attribute_mode);
        ui->cap_setaf = unibi_get_str(ui->ut, unibi_set_a_foreground);
        ui->cap_setab = unibi_get_str(ui->ut, unibi_set_a_background);
    }

    ui->sz = get_term_size();

    g_ui_singleton = ui;
    struct sigaction sa = {0};
    sa.sa_handler = on_sigwinch;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGWINCH, &sa, NULL);

    return ui;
}

void pico_attach(pico_t *ui) {
    /* Make fresh lines for the UI at bottom of current session */
    emit_cup(ui, ui->sz.rows - 1, 0);
    for (int i = 0; i < ui->ui_rows; ++i)
        write_cstr("\r\n");
    pico_redraw_ui(ui);

    /* Confine scroll to top region */
    int bot = ui->sz.rows - ui->ui_rows - 1;
    if (bot < 0)
        bot = 0;
    set_scroll_region(ui, 0, bot);
    move_to_bottom_scroll_line(ui);
}

void pico_set_ui_rows(pico_t *ui, int ui_rows) {
    if (!ui)
        return;
    if (ui_rows < 1)
        ui_rows = 1;
    if (ui_rows >= ui->sz.rows)
        ui_rows = ui->sz.rows - 1;
    if (ui_rows == ui->ui_rows)
        return;

    /* Release region, scroll to make room difference, then reapply */
    reset_scroll_region(ui);

    int delta = ui_rows - ui->ui_rows;
    ui->ui_rows = ui_rows;

    /* If growing UI, push more fresh lines; if shrinking, just redraw lower. */
    if (delta > 0) {
        emit_cup(ui, ui->sz.rows - 1, 0);
        for (int i = 0; i < delta; ++i)
            write_cstr("\r\n");
    }

    reprogram_region_and_redraw(ui);
}

void pico_print(pico_t *ui, const char *text) {
    handle_resize_if_needed(ui);
    move_to_bottom_scroll_line(ui);
    if (text && *text)
        write_cstr(text);
    if (ui->render)
        ui->render(ui, ui->render_ud);
}

void pico_printf(pico_t *ui, const char *fmt, ...) {
    handle_resize_if_needed(ui);
    move_to_bottom_scroll_line(ui);
    va_list ap;
    va_start(ap, fmt);
    write_vprintf(NULL, fmt, ap);
    va_end(ap);
    if (ui->render)
        ui->render(ui, ui->render_ud);
}

void pico_println(pico_t *ui, const char *line) {
    handle_resize_if_needed(ui);
    move_to_bottom_scroll_line(ui);
    if (line && *line)
        write_cstr(line);
    write_cstr("\r\n");
    if (ui->render)
        ui->render(ui, ui->render_ud);
}

void pico_printfln(pico_t *ui, const char *fmt, ...) {
    handle_resize_if_needed(ui);
    move_to_bottom_scroll_line(ui);
    va_list ap;
    va_start(ap, fmt);
    write_vprintf(NULL, fmt, ap);
    va_end(ap);
    write_cstr("\r\n");
    if (ui->render)
        ui->render(ui, ui->render_ud);
}

void pico_print_block(pico_t *ui, const char *block) {
    if (!block)
        return;
    const char *p = block, *start = p;
    while (*p) {
        if (*p == '\n') {
            char tmp[4096];
            size_t len = (size_t)(p - start);
            if (len >= sizeof(tmp))
                len = sizeof(tmp) - 1;
            memcpy(tmp, start, len);
            tmp[len] = '\0';
            pico_println(ui, tmp);
            start = p + 1;
        }
        ++p;
    }
    if (start != p) {
        char tmp[4096];
        size_t len = (size_t)(p - start);
        if (len >= sizeof(tmp))
            len = sizeof(tmp) - 1;
        memcpy(tmp, start, len);
        tmp[len] = '\0';
        pico_println(ui, tmp);
    }
}

void pico_shutdown(pico_t *ui) {
    if (!ui)
        return;
    reset_scroll_region(ui);
    emit_cup(ui, ui->sz.rows - 1, 0);
}

void pico_free(pico_t *ui) {
    if (!ui)
        return;
    if (g_ui_singleton == ui)
        g_ui_singleton = NULL;
    if (ui->ut)
        unibi_destroy(ui->ut);
    free(ui);
}

/* getters */
int pico_ui_rows(const pico_t *ui) { return ui->ui_rows; }
int pico_cols(const pico_t *ui) { return ui->sz.cols; }

static void on_sigint(int sig) {
    (void)sig;
    if (g_ui_singleton) {
        pico_shutdown(g_ui_singleton);
        pico_reset_colors(g_ui_singleton);
    }
    /* Move to last line and flush */
    write_cstr("\n");
    _exit(130); /* conventional Ctrl-C exit code */
}

void pico_install_sigint_handler(pico_t *ui) {
    g_ui_singleton = ui;
    struct sigaction sa = {0};
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
}
