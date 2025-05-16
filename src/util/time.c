#include "util/time.h"

// WINDOWS
#if defined(_WIN32) || defined(_WIN64)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

uint64_t millis_since_start(void) {
    static LARGE_INTEGER first = {0};
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER now;

    if (first.QuadPart == 0) { // first call → init
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&first);
        return 0;
    }

    QueryPerformanceCounter(&now);
    return (uint64_t)((now.QuadPart - first.QuadPart) * 1000ULL /
                      freq.QuadPart);
}

#else // POSIX

#include <time.h>

uint64_t millis_since_start(void) {
    static struct timespec first = {0};
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    /* initialise on very first call */
    if (first.tv_sec == 0 && first.tv_nsec == 0) {
        first = now;
        return 0;
    }

    /* signed differences to avoid wrap-around */
    int64_t ds = (int64_t)now.tv_sec - (int64_t)first.tv_sec;
    int64_t dns = (int64_t)now.tv_nsec - (int64_t)first.tv_nsec;

    /* normalise if nano part went negative */
    if (dns < 0) {
        ds -= 1;
        dns += 1000000000L;
    }

    /* convert to milliseconds */
    return (uint64_t)ds * 1000ULL + (uint64_t)(dns / 1000000L);
}
#endif
