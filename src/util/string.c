#include "util/string.h"

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
