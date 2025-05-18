#include "util/files.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
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
