#include "internal_logging.h"

#include "version.h"

#include "util/os.h"
#include "util/time.h"

#ifdef __APPLE__
#include <sys/syslimits.h>
#else
#include <limits.h>
#endif // __APPLE__
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static FILE *internal_log_file = NULL;

int internal_logging_init() {

    char internal_log_file_path[PATH_MAX];
    char date_time_now[TS_LEN];
    get_date_time_now(date_time_now);

    snprintf(internal_log_file_path, PATH_MAX, "taf_log_internal_%s.log",
             date_time_now);

    internal_log_file = fopen(internal_log_file_path, "w");
    if (!internal_log_file) {
        return -1;
    }

    fputs("TAF INTERNAL LOG START\n", internal_log_file);
    fputs("TAF version " TAF_VERSION "\n", internal_log_file);

    char *os_string = get_os_string();
    fprintf(internal_log_file, "OS: %s\n", os_string);
    fflush(internal_log_file);

    free(os_string);

    return 0;
}

void internal_log(const char *file, int line, const char *func, const char *fmt,
                  ...) {
    if (!internal_log_file)
        return;

    char date_time_now[TS_LEN];
    get_date_time_now(date_time_now);

    // &file[7] - stripping "../src/" part of file path
    fprintf(internal_log_file, "[%s]: [%s/%s : %d]: ", date_time_now, &file[7],
            func, line);

    va_list arg;
    va_start(arg, fmt);
    vfprintf(internal_log_file, fmt, arg);
    va_end(arg);

    fputs("\n", internal_log_file);

    fflush(internal_log_file);
}

void internal_logging_deinit() {
    if (!internal_log_file)
        return;

    fputs("TAF INTERNAL LOG END\n", internal_log_file);

    fflush(internal_log_file);
    fclose(internal_log_file);

    internal_log_file = NULL;
}
