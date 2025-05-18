#include "util/files.h"

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
