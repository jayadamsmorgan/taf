#ifndef TAF_LOG_LEVEL_H
#define TAF_LOG_LEVEL_H

typedef enum {
    TAF_LOG_LEVEL_CRITICAL = 0,
    TAF_LOG_LEVEL_ERROR = 1,
    TAF_LOG_LEVEL_WARNING = 2,
    TAF_LOG_LEVEL_INFO = 3,
    TAF_LOG_LEVEL_DEBUG = 4,
    TAF_LOG_LEVEL_TRACE = 5,
} taf_log_level;

taf_log_level taf_log_level_from_str(const char *str);

const char *taf_log_level_to_str(taf_log_level level);

#endif // TAF_LOG_LEVEL_H
