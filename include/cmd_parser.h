#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#include "test_logs.h"

#include <stdbool.h>

typedef enum {
    CMD_INIT,
    CMD_TEST,
    CMD_LOGS_INFO,
    CMD_HELP,
    CMD_TARGET_ADD,
    CMD_TARGET_REMOVE,
    CMD_UNKNOWN,
} cmd_category;

typedef struct {
    char *project_name;
    bool multitarget;

    bool internal_logging;
} cmd_init_options;

typedef struct {
    // TODO
    bool something;

    bool interactive;
} cmd_config_options;

typedef struct {
    char **tags;
    size_t tags_amount;

    bool no_logs;
    taf_log_level log_level;

    char *target;

    bool internal_logging;
} cmd_test_options;

typedef struct {
    char *arg;

    bool include_outputs;

    bool internal_logging;
} cmd_logs_info_options;

typedef struct {
    char *target;
    bool internal_logging;
} cmd_target_add_options;

typedef struct {
    char *target;
    bool internal_logging;
} cmd_target_remove_options;

cmd_category cmd_parser_parse(int argc, char **argv);

cmd_init_options *cmd_parser_get_init_options();
cmd_config_options *cmd_parser_get_config_options();
cmd_test_options *cmd_parser_get_test_options();
cmd_logs_info_options *cmd_parser_get_logs_info_options();
cmd_target_add_options *cmd_parser_get_target_add_options();
cmd_target_remove_options *cmd_parser_get_target_remove_options();

#endif // CMD_PARSER_H
