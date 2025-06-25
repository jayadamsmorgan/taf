#ifndef UTIL_TIME_H
#define UTIL_TIME_H

#include <stdint.h>

void reset_millis(void);
uint64_t millis_since_start(void);
void reset_taf_start_millis(void);
uint64_t millis_since_taf_start(void);

#define TS_LEN 18 // "MM.DD.YY-HH:mm:ss" + '\0'

void get_date_time_now(char buf[TS_LEN]);

#endif // UTIL_TIME_H
