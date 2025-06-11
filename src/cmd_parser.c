#include "cmd_parser.h"

#include "version.h"

#include "util/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_EQ(STR1, STR2) (!strcmp(STR1, STR2))

static void print_init_help(FILE *file) {
    fprintf(
        file,
        "Usage: taf init project-name [<options>]\n"
        "\n"
        "Initialize new TAF project.\n"
        "\n"
        "Options:\n"
        "  -m, --multitarget        Initialize project for multiple targets\n"
        "  -i, --internal-log       Dump internal logging file\n"
        "  -h, --help               Display help\n");
}

static void print_test_help(FILE *file) {
    fprintf(file,
            "Usage: taf test [<options>]\n"
            "       taf test <target_name> [<options>]\n"
            "\n"
            "Perform project tests.\n"
            "Must specify target name if project "
            "is multitarget.\n"
            "\n"
            "Options:\n"
            "  -l, --log-level <critical|error|warning|info|debug|trace>   Log "
            "level "
            "for TUI output\n"
            "  -n, --no-logs                                               Do "
            "not output "
            "log files after a "
            "test run\n"
            "  -t, --tags <tag1,tag2>                                      "
            "Perform tests "
            "with specified tags\n"
            "  -i, --internal-log                                          "
            "Dump internal logging file\n"
            "  -h, --help                                                  "
            "Display help\n");
}

static void print_logs_help(FILE *file) {
    fprintf(file, "Usage: taf logs [<info>] [<options>]\n"
                  "\n"
                  "Perform actions on TAF logs.\n"
                  "\n"
                  "Categories:\n"
                  "  info               Get information about the test run\n"
                  "  help               Display help\n"
                  "\n"
                  "Options:\n"
                  "  -h, --help         Display help\n");
}

static void print_logs_info_help(FILE *file) {
    fprintf(file, "Usage: taf logs info <latest>\n"
                  "       taf logs info <test_run_raw_json_file>\n"
                  "\n"
                  "Display information about the test run.\n"
                  "\n"
                  "Options:\n"
                  "  -i, --internal-log       Dump internal logging file\n"
                  "  -h, --help               Display help\n");
}

static void print_target_help(FILE *file) {
    fprintf(file,
            "Usage: taf target [<add|remove>]\n"
            "\n"
            "Categories:\n"
            "  add                Add new target to multitarget project\n"
            "  remove             Remove target from multitarget project\n"
            "\n"
            "Options:\n"
            "  -h, --help         Display help\n");
}

static void print_target_add_help(FILE *file) {
    fprintf(file, "Usage: taf target add <target_name>\n"
                  "\n"
                  "Add new target to multitarget project.\n"
                  "\n"
                  "Options:\n"
                  "  -i, --internal-log       Dump internal logging file\n"
                  "  -h, --help               Display help\n");
}

static void print_target_remove_help(FILE *file) {
    fprintf(file, "Usage: taf target remove <target_name>\n"
                  "\n"
                  "Remove target from multitarget project.\n"
                  "\n"
                  "Options:\n"
                  "  -i, --internal-log       Dump internal logging file\n"
                  "  -h, --help               Display help\n");
}

static void print_help(FILE *file) {
    fprintf(file, "Usage: taf [<init|logs|target|test|help|version>]\n"
                  "\n"
                  "TAF Testing Suite.\n"
                  "\n"
                  "Categories:\n"
                  "  init               Initialize new TAF project\n"
                  "  logs               Perform actions on TAF logs\n"
                  "  target             Perform actions on project targets "
                  "(multitarget)\n"
                  "  test               Perform project tests\n"
                  "  help               Display help\n"
                  "  version            Display TAF version\n"
                  "\n"
                  "Options:\n"
                  "  -h, --help         Display help\n"
                  "  -v, --version      Display TAF version\n");
}

static void print_version() {
    fprintf(stdout, "TAF Testing Suite version " TAF_VERSION "\n");
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

static void parse_additional_options(cmd_option *options, int start_index,
                                     int argc, char **argv) {
    if (!options || argc <= start_index || !argv) {
        return;
    }
    int index = start_index;
    while (index < argc) {
        cmd_option *opt = options;
        bool found = false;
        if (argv[index][0] != '-') {
            index++;
            continue;
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

    init_opts.project_name = argv[2];
    init_opts.multitarget = false;
    init_opts.internal_logging = false;

    parse_additional_options(all_init_options, 3, argc, argv);

    return CMD_INIT;
}

static void set_test_tags(const char *arg) {
    // This one has memory leak but it's fine, we should only
    // call it once

    char *copy = strdup(arg);
    size_t sz = strlen(arg) / 2;
    sz = sz == 0 ? 1 : sz;
    // Amount of tags in any case should not be greater
    // than half of amount of characters, e.g. worst case: "t,s,a,g,y,u"
    char *tags[sz];
    for (size_t i = 0; i < sz; i++) {
        tags[i] = NULL;
    }

    test_opts.tags_amount = string_split_by_delim(copy, tags, ",", sz);
    test_opts.tags = malloc(sizeof(char *) * test_opts.tags_amount);
    for (size_t i = 0; i < test_opts.tags_amount; i++) {
        test_opts.tags[i] = strdup(tags[i]);
    }
}

static void set_test_no_logs(const char *) {
    //
    test_opts.no_logs = true;
}

static void set_log_level(const char *arg) {
    taf_log_level log_level = taf_log_level_from_str(arg);
    if (log_level < 0) {
        fprintf(stderr, "Unknown log level %s", arg);
        exit(EXIT_FAILURE);
    }

    test_opts.log_level = log_level;
}

static void get_test_help(const char *) {
    print_test_help(stdout);
    exit(EXIT_SUCCESS);
}

static cmd_option all_test_options[] = {
    {"--log-level", "-l", true, set_log_level},
    {"--no-logs", "-n", false, set_test_no_logs},
    {"--tags", "-t", true, set_test_tags},
    {"--internal-log", "-i", false, set_internal_logging},
    {"--help", "-h", false, get_test_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_test_options(int argc, char **argv) {

    test_opts.tags = NULL;
    test_opts.target = NULL;
    test_opts.tags_amount = 0;
    test_opts.no_logs = false;
    test_opts.log_level = TAF_LOG_LEVEL_INFO;
    test_opts.internal_logging = false;

    if (argc <= 2) {
        return CMD_TEST;
    }

    int index;
    if (argv[2][0] != '-') {
        // Not an option, so must be a target
        test_opts.target = argv[2];
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

static cmd_option all_logs_info_options[] = {
    {"--internal-log", "-i", false, set_internal_logging},
    {"--help", "-h", false, get_logs_info_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_logs_options(int argc, char **argv) {

    if (argc < 3) {
        fprintf(stderr, "'taf logs' requires category [info]\n");
        print_logs_help(stderr);
        return CMD_UNKNOWN;
    }

    if (STR_EQ(argv[2], "info")) {
        if (argc != 4) {
            print_logs_info_help(stderr);
            return CMD_UNKNOWN;
        }
        logs_info_opts.arg = argv[3];
        logs_info_opts.internal_logging = false;
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
        fprintf(stderr, "'taf target' requires category [add|remove]\n");
        print_target_help(stderr);
        return CMD_UNKNOWN;
    }

    if (STR_EQ(argv[2], "add")) {
        if (argc < 4) {
            fprintf(stderr, "'taf target add' requires target_name\n");
            print_target_add_help(stderr);
            return CMD_UNKNOWN;
        }
        target_add_opts.target = argv[3];
        target_add_opts.internal_logging = false;
        parse_additional_options(all_target_add_options, 3, argc, argv);
        return CMD_TARGET_ADD;
    } else if (STR_EQ(argv[2], "remove")) {
        if (argc < 4) {
            fprintf(stderr, "'taf target remove' requires target_name\n");
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

cmd_category cmd_parser_parse(int argc, char **argv) {
    if (argc < 2) {
        print_help(stderr);
        return CMD_UNKNOWN;
    }

    if (STR_EQ(argv[1], "init"))
        return parse_init_options(argc, argv);
    if (STR_EQ(argv[1], "test"))
        return parse_test_options(argc, argv);
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
