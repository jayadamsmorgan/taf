#ifndef INTERNAL_LOGGING_H
#define INTERNAL_LOGGING_H

#define LOG(...) internal_log(__FILE__, __LINE__, __func__, __VA_ARGS__)

int internal_logging_init();
void internal_logging_deinit();

void internal_log(const char *file, int line, const char *func, const char *fmt,
                  ...) __attribute__((format(printf, 4, 5)));

#endif // INTERNAL_LOGGING_H
