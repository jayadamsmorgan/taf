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

    /* Explicit caps for save/restore cursor (used ONLY for the stream area) */
    const char *cap_sav; /* save_cursor */
    const char *cap_res; /* restore_cursor */

    /* Stream (top) anchor uses terminal save/restore slot */
    int stream_anchor_set; /* 0/1 */
    int stream_cur_col;    /* current column on the bottom line */

    int ui_anchor_set; /* 0/1: we have a known UI cursor */
    int ui_cur_row;    /* relative to UI top, 0..ui_rows-1 */
    int ui_cur_col;    /* 0..cols-1 */

    const char *cap_smul; /* enter_underline_mode */
    const char *cap_rmul; /* exit_underline_mode */

    volatile sig_atomic_t resized;
};

static void reprogram_region_and_redraw(pico_t *ui);

static void handle_resize_if_needed(pico_t *ui) {
    if (!ui->resized)
        return;
    ui->resized = 0;
    reprogram_region_and_redraw(ui);
}

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

/* FIX: Never reuse a va_list after it was consumed. Take a copy for sizing,
   and another copy for the real print. */
static void write_vprintf(const char *prefix, const char *fmt, va_list ap) {
    char stack[1024];
    if (prefix)
        write_cstr(prefix);

    va_list ap1;
    va_copy(ap1, ap);
    int n = vsnprintf(stack, sizeof(stack), fmt, ap1);
    va_end(ap1);

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

static void emit_save_cursor(pico_t *ui) {
    if (ui->cap_sav) {
        write_cstr(ui->cap_sav);
        return;
    }
    /* CSI s is widely supported; (DEC 7 is "\x1b7") */
    write_cstr("\x1b[s");
}

static void emit_restore_cursor(pico_t *ui) {
    if (ui->cap_res) {
        write_cstr(ui->cap_res);
        return;
    }
    /* CSI u (DEC 8 would be "\x1b8") */
    write_cstr("\x1b[u");
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
    /* Full screen region */
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

/* ------------ UI cursor math (track our own Y/X; don't use save/restore) ---
 */

/* Advance a column by rendering of ASCII text; tabs advance to 8-column stops.
   We keep it simple: no wide glyphs/combining handling. */
static int advance_col_text(int start_col, const char *s, int term_cols) {
    int col = start_col;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        unsigned char ch = *p;
        if (ch == '\n' || ch == '\r') {
            col = 0;
            continue;
        }
        if (ch == '\t') {
            int next = ((col / 8) + 1) * 8;
            col = next;
            continue;
        }
        if (ch >= 0x20 && ch != 0x7f)
            col += 1;
        if (col >= term_cols) {
            col = term_cols - 1;
            break;
        }
    }
    if (col < 0)
        col = 0;
    return col;
}

static void ui_move_abs(pico_t *ui, int rel_row, int col) {
    int base = ui->sz.rows - ui->ui_rows;
    if (base < 0)
        base = 0;
    int r = base + rel_row;
    if (r < 0)
        r = 0;
    if (r >= ui->sz.rows)
        r = ui->sz.rows - 1;
    if (col < 0)
        col = 0;
    emit_cup(ui, r, col);
}

/* ------------ Public UI API (YX and append variants) -----------------------
 */

void pico_ui_puts_yx(pico_t *ui, int rel_row, int col, const char *s) {
    handle_resize_if_needed(ui); /* FIX: handle resize here too */
    if (!s)
        return;
    if (rel_row < 0)
        rel_row = 0;
    if (col < 0)
        col = 0;

    ui_move_abs(ui, rel_row, col);
    write_cstr(s);

    /* Update our tracked UI cursor */
    ui->ui_anchor_set = 1;
    ui->ui_cur_row = rel_row;
    ui->ui_cur_col = advance_col_text(col, s, ui->sz.cols);
}

void pico_ui_printf_yx(pico_t *ui, int rel_row, int col, const char *fmt, ...) {
    handle_resize_if_needed(ui);
    if (rel_row < 0)
        rel_row = 0;
    if (col < 0)
        col = 0;

    /* Format to stack or heap to compute new col */
    char stack[1024];
    va_list ap1;
    va_start(ap1, fmt);
    int n = vsnprintf(stack, sizeof(stack), fmt, ap1);
    va_end(ap1);

    if (n >= 0 && (size_t)n < sizeof(stack)) {
        ui_move_abs(ui, rel_row, col);
        write_str(stack, (size_t)n);
        ui->ui_anchor_set = 1;
        ui->ui_cur_row = rel_row;
        ui->ui_cur_col = advance_col_text(col, stack, ui->sz.cols);
        return;
    }

    size_t need = (n > 0 ? (size_t)n + 1 : 4096);
    char *buf = malloc(need);
    if (!buf)
        return;

    va_list ap2;
    va_start(ap2, fmt);
    vsnprintf(buf, need, fmt, ap2);
    va_end(ap2);

    ui_move_abs(ui, rel_row, col);
    write_cstr(buf);

    ui->ui_anchor_set = 1;
    ui->ui_cur_row = rel_row;
    ui->ui_cur_col = advance_col_text(col, buf, ui->sz.cols);

    free(buf);
}

/* Ensure we have a starting UI anchor; default to (0,0) in the UI region. */
static void ensure_ui_anchor(pico_t *ui) {
    if (!ui->ui_anchor_set) {
        ui->ui_anchor_set = 1;
        ui->ui_cur_row = 0;
        ui->ui_cur_col = 0;
    }
}

void pico_ui_puts(pico_t *ui, const char *s) {
    handle_resize_if_needed(ui);
    if (!s || !*s)
        return;
    ensure_ui_anchor(ui);
    ui_move_abs(ui, ui->ui_cur_row, ui->ui_cur_col);
    write_cstr(s);
    ui->ui_cur_col = advance_col_text(ui->ui_cur_col, s, ui->sz.cols);
}

void pico_ui_printf(pico_t *ui, const char *fmt, ...) {
    handle_resize_if_needed(ui);
    ensure_ui_anchor(ui);

    char stack[1024];
    va_list ap1;
    va_start(ap1, fmt);
    int n = vsnprintf(stack, sizeof(stack), fmt, ap1);
    va_end(ap1);

    if (n >= 0 && (size_t)n < sizeof(stack)) {
        ui_move_abs(ui, ui->ui_cur_row, ui->ui_cur_col);
        write_str(stack, (size_t)n);
        ui->ui_cur_col = advance_col_text(ui->ui_cur_col, stack, ui->sz.cols);
        return;
    }

    size_t need = (n > 0 ? (size_t)n + 1 : 4096);
    char *buf = malloc(need);
    if (!buf)
        return;

    va_list ap2;
    va_start(ap2, fmt);
    vsnprintf(buf, need, fmt, ap2);
    va_end(ap2);

    ui_move_abs(ui, ui->ui_cur_row, ui->ui_cur_col);
    write_cstr(buf);
    ui->ui_cur_col = advance_col_text(ui->ui_cur_col, buf, ui->sz.cols);

    free(buf);
}

void pico_ui_clear_line(pico_t *ui, int rel_row) {
    handle_resize_if_needed(ui);
    int base = ui->sz.rows - ui->ui_rows;
    if (base < 0)
        base = 0;
    int r = base + (rel_row < 0 ? 0 : rel_row);
    if (r >= ui->sz.rows)
        return;
    emit_cup(ui, r, 0);
    emit_el(ui);
    /* Reset UI cursor to start of that line */
    ui->ui_anchor_set = 1;
    ui->ui_cur_row = (rel_row < 0 ? 0 : rel_row);
    ui->ui_cur_col = 0;
}

void pico_redraw_ui(pico_t *ui) {
    clear_ui_region(ui);
    if (ui->render)
        ui->render(ui, ui->render_ud);
    /* We don't know where you want to continue -> reset UI anchor */
    ui->ui_anchor_set = 0;
}

/* Re-query size, clamp ui_rows, re-apply region, redraw. */
static void reprogram_region_and_redraw(pico_t *ui) {
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

    ui->stream_anchor_set = 0;
    ui->ui_anchor_set = 0;
}

/* ------------ lifecycle ----------------------------------------------------
 */

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
        ui->cap_sav = unibi_get_str(ui->ut, unibi_save_cursor);
        ui->cap_res = unibi_get_str(ui->ut, unibi_restore_cursor);
        ui->cap_smul = unibi_get_str(ui->ut, unibi_enter_underline_mode);
        ui->cap_rmul = unibi_get_str(ui->ut, unibi_exit_underline_mode);
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

    ui->stream_anchor_set = 0;
    ui->ui_anchor_set = 0;
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

    ui->stream_anchor_set = 0;
    ui->ui_anchor_set = 0;

    /* Release region, scroll to make room difference, then reapply */
    reset_scroll_region(ui);

    int delta = ui_rows - ui->ui_rows;
    ui->ui_rows = ui_rows;

    if (delta > 0) {
        emit_cup(ui, ui->sz.rows - 1, 0);
        for (int i = 0; i < delta; ++i)
            write_cstr("\r\n");
    }

    reprogram_region_and_redraw(ui);
}

/* ------------ stream printing (above UI) -----------------------------------
 */

static int stream_bottom_row(const pico_t *ui) {
    int row = ui->sz.rows - ui->ui_rows - 1;
    return row < 0 ? 0 : row;
}

static void ensure_stream_anchor(pico_t *ui) {
    if (!ui->stream_anchor_set) {
        ui->stream_anchor_set = 1;
        ui->stream_cur_col = 0;
    }
}
void pico_print(pico_t *ui, const char *text) {
    handle_resize_if_needed(ui);
    ensure_stream_anchor(ui);

    int row = stream_bottom_row(ui);
    emit_cup(ui, row, ui->stream_cur_col);

    if (text && *text) {
        write_cstr(text);
        ui->stream_cur_col =
            advance_col_text(ui->stream_cur_col, text, ui->sz.cols);
    }

    if (ui->render)
        ui->render(ui, ui->render_ud);
}
void pico_printf(pico_t *ui, const char *fmt, ...) {
    handle_resize_if_needed(ui);
    ensure_stream_anchor(ui);

    char stack[1024];
    va_list ap1;
    va_start(ap1, fmt);
    int n = vsnprintf(stack, sizeof(stack), fmt, ap1);
    va_end(ap1);

    int row = stream_bottom_row(ui);
    emit_cup(ui, row, ui->stream_cur_col);

    if (n >= 0 && (size_t)n < sizeof(stack)) {
        write_str(stack, (size_t)n);
        ui->stream_cur_col =
            advance_col_text(ui->stream_cur_col, stack, ui->sz.cols);
        if (ui->render)
            ui->render(ui, ui->render_ud);
        return;
    }

    size_t need = (n > 0 ? (size_t)n + 1 : 4096);
    char *buf = malloc(need);
    if (!buf) {
        if (ui->render)
            ui->render(ui, ui->render_ud);
        return;
    }

    va_list ap2;
    va_start(ap2, fmt);
    vsnprintf(buf, need, fmt, ap2);
    va_end(ap2);

    write_cstr(buf);
    ui->stream_cur_col = advance_col_text(ui->stream_cur_col, buf, ui->sz.cols);
    free(buf);

    if (ui->render)
        ui->render(ui, ui->render_ud);
}

void pico_println(pico_t *ui, const char *line) {
    handle_resize_if_needed(ui);

    int row = stream_bottom_row(ui);
    emit_cup(ui, row, 0);
    if (line && *line)
        write_cstr(line);
    write_cstr("\r\n");

    /* newline scrolls; next print should re-anchor at column 0 */
    ui->stream_anchor_set = 0;

    if (ui->render)
        ui->render(ui, ui->render_ud);
}

void pico_printfln(pico_t *ui, const char *fmt, ...) {
    handle_resize_if_needed(ui);

    int row = stream_bottom_row(ui);
    emit_cup(ui, row, 0);

    va_list ap;
    va_start(ap, fmt);
    write_vprintf(NULL, fmt, ap);
    va_end(ap);
    write_cstr("\r\n");

    ui->stream_anchor_set = 0;

    if (ui->render)
        ui->render(ui, ui->render_ud);
}
void pico_print_block(pico_t *ui, const char *block) {
    if (!block)
        return;

    handle_resize_if_needed(ui);
    ensure_stream_anchor(ui);

    int row = stream_bottom_row(ui);
    emit_cup(ui, row, ui->stream_cur_col);

    /* Write whole block as-is */
    write_cstr(block);

    /* If block ends with '\n', the cursor moved to next line start:
       let next print re-anchor at col 0. Otherwise, update current col
       based on text after the last newline. */
    size_t len = strlen(block);
    if (len > 0 && block[len - 1] == '\n') {
        ui->stream_anchor_set = 0;
    } else {
        const char *last_nl = strrchr(block, '\n');
        const char *tail = last_nl ? last_nl + 1 : block;
        ui->stream_cur_col =
            advance_col_text(ui->stream_cur_col, tail, ui->sz.cols);
        ui->stream_anchor_set = 1;
    }

    if (ui->render)
        ui->render(ui, ui->render_ud);

    pico_println(ui, "");
}

void pico_underline_on(pico_t *ui) {
    (void)ui;
    /* Prefer terminfo; fallback to SGR 4 */
    if (ui && ui->cap_smul) {
        write_cstr(ui->cap_smul);
    } else {
        write_cstr("\x1b[4m"); /* underline on */
    }
}

void pico_underline_off(pico_t *ui) {
    (void)ui;
    /* Prefer terminfo; fallback to SGR 24 (underline off) */
    if (ui && ui->cap_rmul) {
        write_cstr(ui->cap_rmul);
    } else {
        write_cstr("\x1b[24m"); /* underline off */
    }
}

void pico_remove_cursor()
{
    write_cstr("\033[?25l"); // Turn off cursor
}

void pico_restore_cursor()
{
    printf("\033[?25h"); // Turn on cursor
}
/* ------------ shutdown/free ------------------------------------------------
 */

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

/* NOTE: Signal handlers should be async-signal-safe; this one still calls
   functions that may use snprintf etc. If you need strict safety, consider a
   self-pipe/flag and exit from your main loop instead. */
static void on_sigint(int sig) {
    (void)sig;
    if (g_ui_singleton) {
        pico_shutdown(g_ui_singleton);
        pico_reset_colors(g_ui_singleton);
    }
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
