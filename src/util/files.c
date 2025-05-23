#include "util/files.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool directory_exists(const char *dir) {
    struct stat sb;
    if (stat(dir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        return true;
    }
    return false;
}

bool file_exists(const char *file_path) {
    struct stat sb;
    if (stat(file_path, &sb) == 0 &&
        (S_ISREG(sb.st_mode) || S_ISLNK(sb.st_mode))) {
        return true;
    }
    return false;
}

char *file_find_upwards(const char *filename) {
    if (!filename || !*filename) /* no name given */
        return NULL;

    /* buffer for the current directory; PATH_MAX is a safe upper bound */
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof cwd))
        return NULL;

    for (;;) {
        /* build candidate = "<cwd>/<filename>" */
        char candidate[PATH_MAX];
        if (snprintf(candidate, sizeof candidate, "%s/%s", cwd, filename) >=
            (int)sizeof candidate)
            return NULL; /* would overflow → give up   */

        if (file_exists(candidate))
            return strdup(candidate); /* caller frees                */

        if (strcmp(cwd, "/") == 0)
            break;

        /* Trim one path component: e.g. "/a/b" → "/a"        */
        char *slash = strrchr(cwd, '/');
        if (!slash) /* paranoid safety check      */
            break;
        if (slash == cwd)
            cwd[1] = '\0'; /* keep leading '/' only      */
        else
            *slash = '\0';
    }
    return NULL; /* not found */
}

int create_directory(const char *path, mode_t mode) {
    if (!path || !*path)
        return -1;

    /* Make a writable copy because dirname/basename may modify it */
    char tmp[PATH_MAX];
    if (strlen(path) >= sizeof tmp) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(tmp, path);

    /* Skip leading slash(es) so we handle absolute and relative paths uniformly
     */
    char *p = tmp;
    while (*p == '/')
        ++p;

    /* Iterate over path components, restoring one slash at a time */
    for (; *p; ++p) {
        if (*p != '/')
            continue;
        *p = '\0'; /* temporarily terminate component */

        if (mkdir(tmp, mode) == -1 && errno != EEXIST)
            return -1;

        *p = '/';         /* restore slash */
        while (*p == '/') /* collapse duplicate '/' */
            ++p;
        --p; /* compensate for loop’s ++p      */
    }

    /* Create final component */
    if (mkdir(tmp, mode) == -1 && errno != EEXIST)
        return -1;

    return 0;
}

static int ends_with_lua(const char *name) {
    const char *dot = strrchr(name, '.');
    return dot && strcmp(dot, ".lua") == 0;
}

static int push_string(str_array_t *a, const char *s) {
    char **tmp = realloc(a->items, (a->count + 1) * sizeof *tmp);
    if (!tmp)
        return -1;
    a->items = tmp;

    a->items[a->count] = strdup(s);
    if (!a->items[a->count])
        return -1;

    ++a->count;
    return 0;
}

static int walk(const char *dirpath, str_array_t *out) {
    DIR *dir = opendir(dirpath);
    if (!dir)
        return -1;

    struct dirent *ent;
    while ((ent = readdir(dir))) {
        /* Skip "." and ".." */
        if (ent->d_name[0] == '.' &&
            (ent->d_name[1] == '\0' ||
             (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
            continue;

        char *full = NULL;
        if (asprintf(&full, "%s/%s", dirpath, ent->d_name) == -1) {
            closedir(dir);
            return -1;
        }

        struct stat st;
        if (stat(full, &st) == -1) {
            free(full);
            closedir(dir);
            return -1;
        }

        if (S_ISDIR(st.st_mode)) {
            /* Recurse into sub-directory */
            if (walk(full, out) == -1) {
                free(full);
                closedir(dir);
                return -1;
            }
        } else if ((S_ISREG(st.st_mode) || (S_ISLNK(st.st_mode))) &&
                   ends_with_lua(ent->d_name)) {
            /* Regular *.lua file */
            if (push_string(out, full) == -1) {
                free(full);
                closedir(dir);
                return -1;
            }
        }

        free(full);
    }
    closedir(dir);
    return 0;
}

str_array_t list_lua_recursive(const char *root) {
    str_array_t result = {NULL, 0};
    walk(root, &result);
    return result;
}

void free_str_array(str_array_t *a) {
    if (!a)
        return;
    for (size_t i = 0; i < a->count; ++i)
        free(a->items[i]);
    free(a->items);
    a->items = NULL;
    a->count = 0;
}
