#ifndef CMD_PARSER_H
#define CMD_PARSER_H

typedef enum {
    CMD_CONFIG,
    CMD_INIT,
    CMD_TEST,
    CMD_LOGS,
    CMD_HELP,
    CMD_UNKNOWN,
} cmd_category;

typedef struct {
    const char *project_name;
    bool multitarget;

    bool interactive;
} cmd_init_options;

typedef struct {
    // TODO
    bool something;

    bool interactive;
} cmd_config_options;

typedef struct {
    char **tags;
    int tags_amount;

    char *target;
} cmd_test_options;

typedef enum {
    LOGS_OPT_MERGE,
} cmd_logs_category;

typedef struct {
    cmd_logs_category category;
} cmd_logs_options;

cmd_category cmd_parser_parse(int argc, char **argv);

cmd_init_options *cmd_parser_get_init_options();
cmd_config_options *cmd_parser_get_config_options();
cmd_test_options *cmd_parser_get_test_options();
cmd_logs_options *cmd_parser_get_logs_options();

#endif // CMD_PARSER_H
