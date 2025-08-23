#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include "util/da.h"

#include <stddef.h>

da_t *string_split_by_delim(const char *s, const char *delim);

size_t *string_wrapped_lines(const char *text, size_t max_len, size_t *count);

char *string_join(char *items[], size_t count);

char *string_strip(const char *s);

bool string_has_prefix(const char *str, const char *prefix);

#endif // UTIL_STRING_H
