#include "util/line_cache.h"

#include <stdio.h>
#include <string.h>

static line_cache_t *cache_head = NULL;

char *get_line_text(const char *path, int lineno) {

    for (line_cache_t *c = cache_head; c; c = c->next) {
        if (strcmp(c->path, path) == 0) {
            return c->lines[lineno - 1]; // lineno is 1-based
        }
    }

    FILE *fp = fopen(path, "r");
    if (!fp)
        return NULL;

    line_cache_t *c = malloc(sizeof *c);
    c->path = strdup(path);
    c->lines = NULL;
    c->next = cache_head;
    cache_head = c;

    size_t cap = 0, n = 0;
    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fp) > 0) {
        if (n == cap) {
            cap = cap ? cap * 2 : 128;
            c->lines = realloc(c->lines, sizeof(char *) * (cap + 1));
        }
        c->lines[n++] = strdup(line);
    }
    free(line);
    c->lines[n] = NULL;
    fclose(fp);

    return (lineno > 0 && lineno <= (int)n) ? c->lines[lineno - 1] : NULL;
}
