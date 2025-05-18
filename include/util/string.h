#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stddef.h>

size_t string_split_by_delim(char *s, char *out[], const char *delim,
                             size_t max_fields);

#endif // UTIL_STRING_H
