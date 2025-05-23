#include "util/string.h"

#include <stdlib.h>
#include <string.h>

size_t string_split_by_delim(char *s, char *out[], const char *delim,
                             size_t max_fields) {
    size_t n = 0;

    char *save = NULL;
    for (char *tok = strtok_r(s, delim, &save); tok && n < max_fields;
         tok = strtok_r(NULL, delim, &save)) {
        out[n++] = tok;
    }
    return n;
}

size_t *string_wrapped_lines(const char *text, size_t max_len, size_t *count) {
    if (!text || max_len == 0) {
        if (count)
            *count = 0;
        return NULL;
    }

    size_t col = 0, index = 0, i = 0;
    size_t cap = 8;
    size_t *indices = malloc(cap * sizeof *indices);
    if (!indices) {
        if (count)
            *count = 0;
        return NULL;
    }

    indices[i++] = 0;

    for (const unsigned char *p = (const unsigned char *)text; *p;
         ++p, ++index) {
        if (*p == '\n' || col == max_len) {

            if (p[1] != '\0') {
                if (i == cap) {
                    cap *= 2;
                    size_t *tmp = realloc(indices, cap * sizeof *indices);
                    if (!tmp) {
                        free(indices);
                        if (count)
                            *count = 0;
                        return NULL;
                    }
                    indices = tmp;
                }
                indices[i++] = index + 1;
            }

            col = 0;

            if (*p == '\n')
                continue;
        }
        ++col;
    }

    if (count)
        *count = i;

    size_t *tmp = realloc(indices, i * sizeof *indices);
    if (tmp)
        indices = tmp;

    return indices; /* caller must free() */
}
