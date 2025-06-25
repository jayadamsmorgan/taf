#ifndef UTIL_TIME_H
#define UTIL_TIME_H

void reset_millis(void);
unsigned long millis_since_start(void);
void reset_taf_start_millis(void);
unsigned long millis_since_taf_start(void);

#define TS_LEN 18 // "MM.DD.YY-HH:mm:ss" + '\0'

void get_date_time_now(char buf[TS_LEN]);

#endif // UTIL_TIME_H
