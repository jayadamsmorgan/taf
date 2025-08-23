#ifndef RAW_LOG_H
#define RAW_LOG_H

#include "taf_log_level.h"
#include "util/da.h"

#include <json.h>

#include <stddef.h>

typedef struct {
    char *file;
    int line;
    char *date_time;
    taf_log_level level;
    char *msg;
    size_t msg_len;
} taf_state_test_output_t;

typedef struct {
    char *name;
    char *description;
    char *started;
    char *finished;
    char *teardown_start;
    char *status;

    da_t *tags;

    da_t *failure_reasons;
    da_t *outputs;
    da_t *teardown_outputs;
    da_t *teardown_errors;

} taf_state_test_t;

typedef struct {
    char *project_name;
    char *taf_version;
    char *os;
    char *os_version;
    char *started;
    char *finished;
    char *target;

    size_t total_amount;
    size_t passed_amount;
    size_t failed_amount;
    size_t finished_amount;

    da_t *vars;

    da_t *tags;

    taf_state_test_t *tests;
} taf_state_t;

json_object *taf_state_to_json(taf_state_t *log);

taf_state_t *taf_state_from_json(json_object *obj);

taf_state_t *taf_state_new();

void taf_state_free(taf_state_t *state);

#endif // RAW_LOG_H
