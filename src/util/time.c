#include "util/time.h"

#include <stdint.h>

// WINDOWS
#if defined(_WIN32) || defined(_WIN64)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static LARGE_INTEGER first = {0};
static LARGE_INTEGER freq = {0};

void reset_millis(void) {
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&first);
}

uint64_t millis_since_start(void) {
    LARGE_INTEGER now;

    QueryPerformanceCounter(&now);
    return (uint64_t)((now.QuadPart - first.QuadPart) * 1000ULL /
                      freq.QuadPart);
}

#else // POSIX

#include <time.h>

static struct timespec first = {0};
static struct timespec taf_start = {0};

void reset_millis(void) {
    //
    clock_gettime(CLOCK_MONOTONIC, &first);
}

unsigned long millis_since_start(void) {
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    /* signed differences to avoid wrap-around */
    int64_t ds = (int64_t)now.tv_sec - (int64_t)first.tv_sec;
    int64_t dns = (int64_t)now.tv_nsec - (int64_t)first.tv_nsec;

    /* normalise if nano part went negative */
    if (dns < 0) {
        ds -= 1;
        dns += 1000000000L;
    }

    /* convert to milliseconds */
    return (unsigned long)ds * 1000ULL + (unsigned long)(dns / 1000000L);
}

void reset_taf_start_millis(void) {
    //
    clock_gettime(CLOCK_MONOTONIC, &taf_start);
}

unsigned long millis_since_taf_start(void) {
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    /* signed differences to avoid wrap-around */
    int64_t ds = (int64_t)now.tv_sec - (int64_t)taf_start.tv_sec;
    int64_t dns = (int64_t)now.tv_nsec - (int64_t)taf_start.tv_nsec;

    /* normalise if nano part went negative */
    if (dns < 0) {
        ds -= 1;
        dns += 1000000000L;
    }

    /* convert to milliseconds */
    return (unsigned long)ds * 1000ULL + (unsigned long)(dns / 1000000L);
}
#endif

void get_date_time_now(char buf[TS_LEN]) {
    time_t raw = time(NULL);
    struct tm *tmnow = localtime(&raw);
    strftime(buf, TS_LEN, "%m.%d.%y-%H:%M:%S", tmnow);
}
