#ifndef LINE_CACHE_H
#define LINE_CACHE_H

#include <stdlib.h>

typedef struct line_cache {
    char *path;
    char **lines;
    struct line_cache *next;
} line_cache_t;

char *get_line_text(const char *path, int lineno);

#endif // LINE_CACHE_H
