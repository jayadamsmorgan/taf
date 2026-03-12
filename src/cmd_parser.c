#include "cmd_parser.h"

#include "ltf_log_level.h"
#include "util/da.h"
#include "util/kv.h"
#include "version.h"

#include "util/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_EQ(STR1, STR2) (!strcmp(STR1, STR2))

static void print_init_help(FILE *file) {
    fprintf(
        file,
        "Usage: ltf init project-name [<options>]\n"
        "\n"
        "Initialize new LTF project.\n"
        "\n"
        "Options:\n"
        "  -m, --multitarget        Initialize project for multiple targets\n"
        "  -i, --internal-log       Dump internal logging file\n"
        "  -h, --help               Display help\n");
}

static void print_test_help(FILE *file) {
    fprintf(file,
            "Usage: ltf test [<options>]\n"
            "       ltf test <target_name> [<options>]\n"
            "\n"
            "Perform project tests.\n"
            "Must specify target name if project is multitarget.\n"
            "\n"
            "Options:\n"
            "  -l, --log-level <critical|error|warning|info|debug|trace>   "
            "Log level for TUI output\n"
            "  -n, --no-logs                                               "
            "Do not output log files after a test run\n"
            "  --skip-hooks                                                "
            "Skip LTF hooks execution for the test run\n"
            "  -p, --ltf-lib-path <path>                                   "
            "Specify custom path to LTF Lua libraries\n"
            "  -t, --tags <tag1,tag2>                                      "
            "Perform tests "
            "with specified tags\n"
            "  -v, --vars <var1:value1,var2:value2>                        "
            "Specify custom variables\n"
            "  -s, --scenario <file.json>                                  "
            "Specify test scenario file\n"
            "  -i, --internal-log                                          "
            "Dump internal logging file\n"
            "  -e, --headless                                              "
            "Run in headless mode (no TUI)\n"
            "  -h, --help                                                  "
            "Display help\n");
}

static void print_logs_help(FILE *file) {
    fprintf(file, "Usage: ltf logs [<info>] [<options>]\n"
                  "\n"
                  "Perform actions on LTF logs.\n"
                  "\n"
                  "Categories:\n"
                  "  info               Get information about the test run\n"
                  "  help               Display help\n"
                  "\n"
                  "Options:\n"
                  "  -h, --help         Display help\n");
}

static void print_logs_info_help(FILE *file) {
    fprintf(file, "Usage: ltf logs info <latest>\n"
                  "       ltf logs info <test_run_raw_json_file>\n"
                  "\n"
                  "Display information about the test run.\n"
                  "\n"
                  "Options:\n"
                  "  -o, --outputs            Include outputs\n"
                  "  -i, --internal-log       Dump internal logging file\n"
                  "  -k, --keyword-tree       Draw tests keyword trees\n"
                  "  -h, --help               Display help\n");
}

static void print_target_help(FILE *file) {
    fprintf(file,
            "Usage: ltf target [<add|remove>]\n"
            "\n"
            "Categories:\n"
            "  add                Add new target to multitarget project\n"
            "  remove             Remove target from multitarget project\n"
            "\n"
            "Options:\n"
            "  -h, --help         Display help\n");
}

static void print_target_add_help(FILE *file) {
    fprintf(file, "Usage: ltf target add <target_name>\n"
                  "\n"
                  "Add new target to multitarget project.\n"
                  "\n"
                  "Options:\n"
                  "  -i, --internal-log       Dump internal logging file\n"
                  "  -h, --help               Display help\n");
}

static void print_target_remove_help(FILE *file) {
    fprintf(file, "Usage: ltf target remove <target_name>\n"
                  "\n"
                  "Remove target from multitarget project.\n"
                  "\n"
                  "Options:\n"
                  "  -i, --internal-log       Dump internal logging file\n"
                  "  -h, --help               Display help\n");
}

static void print_eval_help(FILE *file) {
    fprintf(file, "Usage: ltf eval <file.lua> [<options>] -- [args...]\n"
                  "\n"
                  "Run Lua scripts with LTF libraries\n"
                  "\n"
                  "Options:\n"
                  "  -i, --internal-log       Dump internal logging file\n"
                  "  -h, --help               Display help\n");
}

static void print_help(FILE *file) {
    fprintf(file, "Usage: ltf [<init|logs|target|test|help|version>]\n"
                  "\n"
                  "LTF Testing Suite.\n"
                  "\n"
                  "Categories:\n"
                  "  init               Initialize new LTF project\n"
                  "  logs               Perform actions on LTF logs\n"
                  "  target             Perform actions on project targets "
                  "(multitarget)\n"
                  "  test               Perform project tests\n"
                  "  eval               Run Lua scripts with LTF libraries\n"
                  "  help               Display help\n"
                  "  version            Display LTF version\n"
                  "\n"
                  "Options:\n"
                  "  -h, --help         Display help\n"
                  "  -v, --version      Display LTF version\n");
}

static void print_version() {
    fprintf(stdout, "LTF Testing Suite version " LTF_VERSION "\n");
}

static cmd_init_options init_opts;
cmd_init_options *cmd_parser_get_init_options() {
    //
    return &init_opts;
}

static cmd_config_options config_opts;
cmd_config_options *cmd_parser_get_config_options() {
    //
    return &config_opts;
}

static cmd_test_options test_opts;
cmd_test_options *cmd_parser_get_test_options() {
    //
    return &test_opts;
}

static cmd_target_add_options target_add_opts;
cmd_target_add_options *cmd_parser_get_target_add_options() {
    //
    return &target_add_opts;
}

static cmd_target_remove_options target_remove_opts;
cmd_target_remove_options *cmd_parser_get_target_remove_options() {
    //
    return &target_remove_opts;
}

static cmd_logs_info_options logs_info_opts;
cmd_logs_info_options *cmd_parser_get_logs_info_options() {
    //
    return &logs_info_opts;
}

static cmd_eval_options eval_opts;
cmd_eval_options *cmd_parser_get_eval_options() {
    //
    return &eval_opts;
}

typedef struct {
    const char *long_opt;
    const char *short_opt;
    bool has_argument;
    void (*callback)(const char *arg);
} cmd_option;

static bool is_option_present(cmd_option *opt, const char *opt_str, int index,
                              int argc, char **argv) {
    if (!opt_str) {
        return false;
    }
    if (!STR_EQ(opt_str, argv[index])) {
        return false;
    }

    const char *arg = NULL;

    if (opt->has_argument) {
        if (index + 1 >= argc) {
            fprintf(stderr, "Option %s requires an argument.", argv[index]);
            exit(EXIT_FAILURE);
        }
        arg = argv[index + 1];
    }

    if (opt->callback) {
        opt->callback(arg);
    }

    return true;
}

static int parse_additional_options(cmd_option *options, int start_index,
                                    int argc, char **argv) {
    if (!options || argc <= start_index || !argv) {
        return -1;
    }
    int index = start_index;
    while (index < argc) {
        cmd_option *opt = options;
        bool found = false;
        if (argv[index][0] != '-') {
            index++;
            continue;
        }
        if (STR_EQ(argv[index], "--")) {
            return index + 1;
        }
        while (opt && (opt->long_opt || opt->short_opt)) {
            if (is_option_present(opt, opt->long_opt, index, argc, argv)) {
                found = true;
                break;
            }
            if (is_option_present(opt, opt->short_opt, index, argc, argv)) {
                found = true;
                break;
            }
            opt++;
        }
        if (!found) {
            fprintf(stderr, "Unknown option %s\n", argv[index]);
            exit(EXIT_FAILURE);
        }
        if (opt->has_argument) {
            index++;
        }
        index++;
    }
    return 0;
}

static void set_init_multitarget(const char *) {
    //
    init_opts.multitarget = true;
}

static void get_init_help(const char *) {
    print_init_help(stdout);
    exit(EXIT_SUCCESS);
}

static void set_internal_logging(const char *) {
    init_opts.internal_logging = true;
    target_add_opts.internal_logging = true;
    target_remove_opts.internal_logging = true;
    test_opts.internal_logging = true;
    logs_info_opts.internal_logging = true;
    eval_opts.internal_logging = true;
}

static cmd_option all_init_options[] = {
    {"--multitarget", "-m", false, set_init_multitarget},
    {"--internal-log", "-i", false, set_internal_logging},
    {"--help", "-h", false, get_init_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_init_options(int argc, char **argv) {
    if (argc <= 2) {
        print_init_help(stderr);
        return CMD_UNKNOWN;
    }

    if (STR_EQ(argv[2], "--help") || STR_EQ(argv[2], "-h")) {
        print_init_help(stdout);
        return CMD_HELP;
    }

    init_opts.project_name = strdup(argv[2]);
    init_opts.multitarget = false;
    init_opts.internal_logging = false;

    parse_additional_options(all_init_options, 3, argc, argv);

    return CMD_INIT;
}

static void set_test_tags(const char *arg) {
    da_t *tags = string_split_by_delim(arg, ",");
    if (!tags) {
        fprintf(stderr, "Unknown error: Unable to set test tags.");
        exit(EXIT_FAILURE);
    }
    size_t sz = da_size(test_opts.tags);
    if (sz) {
        for (size_t i = 0; i < sz; ++i) {
            char **tag = da_get(test_opts.tags, i);
            free(*tag);
        }
        da_free(test_opts.tags);
        test_opts.tags = da_init(1, sizeof(kv_pair_t));
    }
    for (size_t i = 0; i < da_size(tags); ++i) {
        da_append(test_opts.tags, da_get(tags, i));
    }
    da_free(tags);
}

static void set_test_vars(const char *arg) {
    char *copy = strdup(arg);
    if (!copy) {
        fprintf(stderr, "parse_vars: strdup failed (out of memory)\n");
        exit(EXIT_FAILURE);
    }

    char *token = strtok(copy, ",");
    while (token) {
        char *eq = strchr(token, '=');
        if (!eq || *(eq + 1) == '\0') {
            fprintf(stderr,
                    "parse_vars: malformed argument '%s' "
                    "(missing '=' or value)\n",
                    token);

            free(copy);
            exit(EXIT_FAILURE);
        }

        *eq = '\0'; // split argname and value

        kv_pair_t pair = {
            .key = strdup(token),
            .value = strdup(eq + 1),
        };

        bool found = false;
        for (size_t i = 0; i < da_size(test_opts.vars); ++i) {
            kv_pair_t *var = da_get(test_opts.vars, i);
            if (STR_EQ(pair.key, var->key)) {
                free(var->key);
                free(var->value);
                var->key = pair.key;
                var->value = pair.value;
                found = true;
                break;
            }
        }

        if (!found) {
            da_append(test_opts.vars, &pair);
        }

        token = strtok(NULL, ",");
    }

    free(copy);
}

static void set_test_no_logs(const char *) {
    //
    test_opts.no_logs = true;
}

static void set_log_level(const char *arg) {
    ltf_log_level log_level = ltf_log_level_from_str(arg);
    if (log_level < 0) {
        fprintf(stderr, "Unknown log level %s", arg);
        exit(EXIT_FAILURE);
    }

    test_opts.log_level_set = true;
    test_opts.log_level = log_level;
}

static void get_test_help(const char *) {
    print_test_help(stdout);
    exit(EXIT_SUCCESS);
}

static void set_test_ltf_lib_path(const char *arg) {
    free(test_opts.custom_ltf_lib_path);
    test_opts.custom_ltf_lib_path = strdup(arg);
}

static void set_test_headless(const char *) {
    //
    test_opts.headless = true;
}

static void set_test_scenario(const char *arg) {
    int res = ltf_test_scenario_parse(arg, &test_opts.scenario);
    if (res) {
        exit(EXIT_FAILURE);
    }
    test_opts.scenario_parsed = true;
    ltf_test_scenario_parsed_t *sc = &test_opts.scenario;

    for (size_t i = 0; i < da_size(sc->vars); ++i) {
        bool found = false;
        for (size_t j = 0; j < da_size(test_opts.vars); ++j) {
            kv_pair_t *scenario_var = da_get(sc->vars, i);
            kv_pair_t *reg_var = da_get(test_opts.vars, j);
            if (STR_EQ(scenario_var->key, reg_var->key)) {
                // Giving priority to CLI
                free(scenario_var->value);
                scenario_var->value = NULL;
                free(scenario_var->key);
                scenario_var->key = NULL;
                found = true;
                break;
            }
        }
        if (!found) {
            da_append(test_opts.vars, da_get(sc->vars, i));
        }
    }

    if (da_size(test_opts.tags) == 0) {
        for (size_t i = 0; i < da_size(sc->tags); ++i) {
            da_append(test_opts.tags, da_get(sc->tags, i));
        }
    }

    if (sc->target) {
        free(test_opts.target);
        test_opts.target = strdup(sc->target);
    }
    if (!test_opts.log_level_set) {
        test_opts.log_level = sc->log_level;
    }
    if (!test_opts.custom_ltf_lib_path && sc->ltf_lib_path) {
        test_opts.custom_ltf_lib_path = strdup(sc->ltf_lib_path);
    }
    if (!test_opts.skip_hooks) {
        test_opts.skip_hooks = sc->skip_hooks;
    }
    if (!test_opts.headless) {
        test_opts.headless = sc->headless;
    }
    if (!test_opts.no_logs) {
        test_opts.no_logs = sc->no_logs;
    }
}

static void set_skip_hooks(const char *) {
    //
    test_opts.skip_hooks = true;
}

static cmd_option all_test_options[] = {
    {"--log-level", "-l", true, set_log_level},
    {"--skip-hooks", NULL, false, set_skip_hooks},
    {"--no-logs", "-n", false, set_test_no_logs},
    {"--ltf-lib-path", "-p", true, set_test_ltf_lib_path},
    {"--tags", "-t", true, set_test_tags},
    {"--vars", "-v", true, set_test_vars},
    {"--scenario", "-s", true, set_test_scenario},
    {"--internal-log", "-i", false, set_internal_logging},
    {"--headless", "-e", false, set_test_headless},
    {"--help", "-h", false, get_test_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_test_options(int argc, char **argv) {

    test_opts.tags = da_init(1, sizeof(char *));
    test_opts.target = NULL;
    test_opts.no_logs = false;
    test_opts.log_level_set = false;
    test_opts.log_level = LTF_LOG_LEVEL_INFO;
    test_opts.internal_logging = false;
    test_opts.custom_ltf_lib_path = NULL;
    test_opts.headless = NULL;
    test_opts.skip_hooks = false;
    test_opts.vars = da_init(1, sizeof(kv_pair_t));
    memset(&test_opts.scenario, 0, sizeof(test_opts.scenario));
    test_opts.scenario_parsed = false;

    if (argc <= 2) {
        return CMD_TEST;
    }

    int index;
    if (argv[2][0] != '-') {
        // Not an option, so must be a target
        test_opts.target = strdup(argv[2]);
        index = 3;
    } else {
        index = 2;
    }
    parse_additional_options(all_test_options, index, argc, argv);

    return CMD_TEST;
}

static void get_logs_info_help(const char *) {
    print_logs_info_help(stdout);
    exit(EXIT_SUCCESS);
}

static void set_logs_info_outputs(const char *) {
    logs_info_opts.include_outputs = true;
}

static void set_logs_info_keyword_tree(const char *) {
    logs_info_opts.keyword_tree = true;
}

static cmd_option all_logs_info_options[] = {
    {"--internal-log", "-i", false, set_internal_logging},
    {"--keyword-tree", "-k", false, set_logs_info_keyword_tree},
    {"--outputs", "-o", false, set_logs_info_outputs},
    {"--help", "-h", false, get_logs_info_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_logs_options(int argc, char **argv) {

    if (argc < 3) {
        fprintf(stderr, "'ltf logs' requires category [info]\n");
        print_logs_help(stderr);
        return CMD_UNKNOWN;
    }

    if (STR_EQ(argv[2], "info")) {
        if (argc < 4) {
            print_logs_info_help(stderr);
            return CMD_UNKNOWN;
        }
        logs_info_opts.arg = argv[3];
        logs_info_opts.internal_logging = false;
        logs_info_opts.include_outputs = false;
        logs_info_opts.keyword_tree = false;
        parse_additional_options(all_logs_info_options, 3, argc, argv);
        return CMD_LOGS_INFO;
    } else if (STR_EQ(argv[2], "help") || STR_EQ(argv[2], "-h") ||
               STR_EQ(argv[2], "--help")) {
        print_logs_help(stdout);
        return CMD_HELP;
    }

    fprintf(stderr, "Unknown logs category %s\n", argv[2]);
    print_logs_help(stderr);
    return CMD_UNKNOWN;
}

static void get_target_add_help(const char *) {
    print_target_add_help(stdout);
    exit(EXIT_SUCCESS);
}

static cmd_option all_target_add_options[] = {
    {"--internal-log", "-i", false, set_internal_logging},
    {"--help", "-h", false, get_target_add_help},
    {NULL, NULL, false, NULL},
};

static void get_target_remove_help(const char *) {
    print_target_remove_help(stdout);
    exit(EXIT_SUCCESS);
}

static cmd_option all_target_remove_options[] = {
    {"--internal-log", "-i", false, set_internal_logging},
    {"--help", "-h", false, get_target_remove_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_target_options(int argc, char **argv) {

    if (argc < 3) {
        fprintf(stderr, "'ltf target' requires category [add|remove]\n");
        print_target_help(stderr);
        return CMD_UNKNOWN;
    }

    if (STR_EQ(argv[2], "add")) {
        if (argc < 4) {
            fprintf(stderr, "'ltf target add' requires target_name\n");
            print_target_add_help(stderr);
            return CMD_UNKNOWN;
        }
        target_add_opts.target = argv[3];
        target_add_opts.internal_logging = false;
        parse_additional_options(all_target_add_options, 3, argc, argv);
        return CMD_TARGET_ADD;
    } else if (STR_EQ(argv[2], "remove")) {
        if (argc < 4) {
            fprintf(stderr, "'ltf target remove' requires target_name\n");
            print_target_remove_help(stderr);
            return CMD_UNKNOWN;
        }
        target_remove_opts.target = argv[3];
        target_remove_opts.internal_logging = false;
        parse_additional_options(all_target_remove_options, 3, argc, argv);
        return CMD_TARGET_REMOVE;
    } else if (STR_EQ(argv[2], "--help") || STR_EQ(argv[2], "-h")) {
        print_target_help(stdout);
        return CMD_HELP;
    }

    fprintf(stderr, "Unknown target category %s\n", argv[2]);
    print_target_help(stderr);
    return CMD_UNKNOWN;
}

static void get_eval_help(const char *) {
    print_eval_help(stdout);
    exit(EXIT_SUCCESS);
}

static cmd_option all_eval_options[] = {
    {"--internal-log", "-i", false, set_internal_logging},
    {"--help", "-h", false, get_eval_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_eval_options(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "'ltf eval' requires lua script name\n");
        print_eval_help(stderr);
        return CMD_UNKNOWN;
    }

    if (STR_EQ(argv[2], "--help") || STR_EQ(argv[2], "-h")) {
        print_eval_help(stdout);
        return CMD_HELP;
    }

    eval_opts.name = argv[2];
    eval_opts.args = NULL;
    eval_opts.internal_logging = false;

    if (argc == 3) {
        return CMD_EVAL;
    }

    int res = parse_additional_options(all_eval_options, 3, argc, argv);

    if (res > 3) {
        eval_opts.args = da_init(1, sizeof(char *));
        for (int i = res; i < argc; ++i) {
            const char *arg = strdup(argv[i]);
            da_append(eval_opts.args, &arg);
        }
    }

    return CMD_EVAL;
}

cmd_category cmd_parser_parse(int argc, char **argv) {
    if (argc < 2) {
        print_help(stderr);
        return CMD_UNKNOWN;
    }

    if (STR_EQ(argv[1], "init"))
        return parse_init_options(argc, argv);
    if (STR_EQ(argv[1], "test"))
        return parse_test_options(argc, argv);
    if (STR_EQ(argv[1], "eval"))
        return parse_eval_options(argc, argv);
    if (STR_EQ(argv[1], "logs"))
        return parse_logs_options(argc, argv);
    if (STR_EQ(argv[1], "target"))
        return parse_target_options(argc, argv);

    if (STR_EQ(argv[1], "help") || STR_EQ(argv[1], "--help") ||
        STR_EQ(argv[1], "-h")) {
        print_help(stdout);
        return CMD_HELP;
    }

    if (STR_EQ(argv[1], "version") || STR_EQ(argv[1], "--version") ||
        STR_EQ(argv[1], "-v")) {
        print_version();
        return CMD_HELP;
    }

    fprintf(stderr, "Unknown option %s\n", argv[1]);
    print_help(stderr);
    return CMD_UNKNOWN;
}

void cmd_parser_free_init_options() {
    //
    free(init_opts.project_name);
}

static void free_str_da(da_t *da) {
    size_t sz = da_size(da);
    for (size_t i = 0; i < sz; ++i) {
        char **str = da_get(da, i);
        free(*str);
    }
    da_free(da);
}

static void free_kv_pair_da(da_t *da) {
    size_t sz = da_size(da);
    for (size_t i = 0; i < sz; ++i) {
        kv_pair_t *pair = da_get(da, i);
        free(pair->key);
        free(pair->value);
    }
    da_free(da);
}

void cmd_parser_free_test_options() {
    free_str_da(test_opts.tags);
    free_kv_pair_da(test_opts.vars);
    free(test_opts.target);
    free(test_opts.custom_ltf_lib_path);

    free(test_opts.scenario.target);
    free_str_da(test_opts.scenario.tags);
    free_kv_pair_da(test_opts.scenario.vars);
    free(test_opts.scenario.ltf_lib_path);
    free_str_da(test_opts.scenario.order);
}

void cmd_parser_free_eval_options() {
    //
    free_str_da(eval_opts.args);
}
