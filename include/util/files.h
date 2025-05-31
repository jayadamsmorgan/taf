#ifndef UTIL_FILES_H
#define UTIL_FILES_H

#include <stdbool.h>
#include <stdlib.h>

#define MKDIR_MODE 0700

bool directory_exists(const char *dir);

int create_directory(const char *path, mode_t mode);

bool file_exists(const char *file_path);

void replace_symlink(const char *target, const char *linkname);

char *file_find_upwards(const char *filename);

typedef struct {
    char **items;
    size_t count;
} str_array_t;

str_array_t list_lua_recursive(const char *root);

void free_str_array(str_array_t *a);

#endif // UTIL_FILES_H
