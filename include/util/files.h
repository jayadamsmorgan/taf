#ifndef UTIL_FILES_H
#define UTIL_FILES_H

#include <stdbool.h>
#include <stdlib.h>

bool directory_exists(const char *dir);

int create_directory(const char *path, mode_t mode);

bool file_exists(const char *file_path);

char *file_find_upwards(const char *filename);

#endif // UTIL_FILES_H
