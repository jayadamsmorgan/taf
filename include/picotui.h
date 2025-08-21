#ifndef PICOTUI_H
#define PICOTUI_H

#include <stdarg.h>
#include <stddef.h>

typedef struct pico_t pico_t;

/* Render callback: draw inside the reserved bottom UI region. */
typedef void (*pico_render_fn)(pico_t *ui, void *userdata);

/* Create with initial ui_rows. */
pico_t *pico_init(int ui_rows, pico_render_fn render, void *userdata);

/* Attach to current terminal; sets scroll region and draws UI. */
void pico_attach(pico_t *ui);

/* Install a default SIGINT (Ctrl-C) handler that cleans up and exits.
   Call once after pico_attach(). */
void pico_install_sigint_handler(pico_t *ui);

/* Change the height of the UI region dynamically.
   Clamped to [1, rows-1]. Reapplies scroll region and redraws UI. */
void pico_set_ui_rows(pico_t *ui, int ui_rows);

/* Print text above UI without automatic newline */
void pico_print(pico_t *ui, const char *text);

/* printf-style version (no newline) */
void pico_printf(pico_t *ui, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/* Print one logical line above the UI (adds newline). */
void pico_println(pico_t *ui, const char *line);

/* printf-style streaming line above the UI (adds newline). */
void pico_printfln(pico_t *ui, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/* Print a block (may contain '\n'); split & print per line. */
void pico_print_block(pico_t *ui, const char *block);

/* Force UI redraw (calls render callback). */
void pico_redraw_ui(pico_t *ui);

/* Shutdown and free. */
void pico_shutdown(pico_t *ui);
void pico_free(pico_t *ui);

/* Helpers available to render callback */
int pico_ui_rows(const pico_t *ui);
int pico_cols(const pico_t *ui);

/* UI-region writes */
void pico_ui_puts(pico_t *ui, int rel_row, int col, const char *s);
void pico_ui_clear_line(pico_t *ui, int rel_row);

/* printf-style UI text at (rel_row, col) */
void pico_ui_printf(pico_t *ui, int rel_row, int col, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

/* 16-color palette indices for pico_set_colors() */
#define PICO_COLOR_BLACK 0
#define PICO_COLOR_RED 1
#define PICO_COLOR_GREEN 2
#define PICO_COLOR_YELLOW 3
#define PICO_COLOR_BLUE 4
#define PICO_COLOR_MAGENTA 5
#define PICO_COLOR_CYAN 6
#define PICO_COLOR_WHITE 7

#define PICO_COLOR_BRIGHT_BLACK 8
#define PICO_COLOR_BRIGHT_RED 9
#define PICO_COLOR_BRIGHT_GREEN 10
#define PICO_COLOR_BRIGHT_YELLOW 11
#define PICO_COLOR_BRIGHT_BLUE 12
#define PICO_COLOR_BRIGHT_MAGENTA 13
#define PICO_COLOR_BRIGHT_CYAN 14
#define PICO_COLOR_BRIGHT_WHITE 15

/* --- 16-color palette control (standard 0..7, bright 8..15) ---
   fg/bg: pass -1 to skip changing that channel.
   Returns 0 on success (best-effort). */
int pico_set_colors(pico_t *ui, int fg16, int bg16);

static inline int pico_set_fg(pico_t *ui, int fg16) {
    return pico_set_colors(ui, fg16, -1);
}

static inline int pico_set_bg(pico_t *ui, int bg16) {
    return pico_set_colors(ui, -1, bg16);
}

void pico_reset_colors(pico_t *ui); /* resets attributes (SGR 0) */

#endif // PICOTUI_H
