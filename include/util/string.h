#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stddef.h>

size_t string_split_by_delim(char *s, char *out[], const char *delim,
                             size_t max_fields);

size_t *string_wrapped_lines(const char *text, size_t max_len, size_t *count);

#endif // UTIL_STRING_H
