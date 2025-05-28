#include "cmd_parser.h"
#include "util/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_EQ(STR1, STR2) (!strcmp(STR1, STR2))

static void print_config_help(FILE *file) {
    fprintf(file, "Usage: taf config [--something]\n");
}

static void print_init_help(FILE *file) {
    fprintf(file, "Usage: taf init project-name [--multitarget]\n");
}

static void print_test_help(FILE *file) {
    fprintf(file, "Usage: taf test [--tags tag1,tag2,tag3]\n");
}

static void print_logs_help(FILE *file) {
    fprintf(file, "Usage: taf logs [merge]\n");
}

static void print_help(FILE *file) {
    fprintf(file, "Usage: taf [config|init|test|logs]\n");
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

static cmd_logs_options logs_opts;
cmd_logs_options *cmd_parser_get_logs_options() {
    //
    return &logs_opts;
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

static cmd_option all_init_options[] = {
    {"--multitarget", "-m", false, set_init_multitarget},
    {"--help", "-h", false, get_init_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_init_options(int argc, char **argv) {
    if (argc <= 2) {
        print_init_help(stderr);
        return CMD_UNKNOWN;
    }

    init_opts.project_name = argv[2];
    init_opts.multitarget = false;

    parse_additional_options(all_init_options, 3, argc, argv);

    return CMD_INIT;
}

static void get_config_help(const char *) {
    print_config_help(stdout);
    exit(EXIT_SUCCESS);
}

static cmd_option all_config_options[] = {
    {"--help", "-h", false, get_config_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_config_options(int argc, char **argv) {
    // TODO

    config_opts.something = true;
    config_opts.interactive = true;

    parse_additional_options(all_config_options, 2, argc, argv);

    return CMD_CONFIG;
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

static void get_test_help(const char *) {
    print_test_help(stdout);
    exit(EXIT_SUCCESS);
}

static cmd_option all_test_options[] = {
    {"--tags", "-t", true, set_test_tags},
    {"--help", "-h", false, get_test_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_test_options(int argc, char **argv) {

    test_opts.tags = NULL;
    test_opts.target = NULL;
    test_opts.tags_amount = 0;

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

static void get_logs_help(const char *) {
    print_logs_help(stdout);
    exit(EXIT_SUCCESS);
}

static cmd_option all_logs_options[] = {
    {"--help", "-h", false, get_logs_help},
    {NULL, NULL, false, NULL},
};

static cmd_category parse_logs_options(int argc, char **argv) {

    if (argc < 3) {
        fprintf(stderr, "taf logs requires category (merge)\n");
        print_logs_help(stderr);
        return CMD_UNKNOWN;
    }

    if (STR_EQ(argv[2], "merge")) {
        logs_opts.category = LOGS_OPT_MERGE;
    } else {
        fprintf(stderr, "Unknown logs category %s\n", argv[2]);
        print_logs_help(stderr);
        return CMD_UNKNOWN;
    }

    parse_additional_options(all_logs_options, 3, argc, argv);

    return CMD_LOGS;
}

cmd_category cmd_parser_parse(int argc, char **argv) {
    if (argc < 2) {
        print_help(stderr);
        return CMD_UNKNOWN;
    }

    if (STR_EQ(argv[1], "config"))
        return parse_config_options(argc, argv);
    if (STR_EQ(argv[1], "init"))
        return parse_init_options(argc, argv);
    if (STR_EQ(argv[1], "test"))
        return parse_test_options(argc, argv);
    if (STR_EQ(argv[1], "logs"))
        return parse_logs_options(argc, argv);

    if (STR_EQ(argv[1], "help") || STR_EQ(argv[1], "--help") ||
        STR_EQ(argv[1], "-h")) {
        print_help(stdout);
        return CMD_HELP;
    }

    fprintf(stderr, "Unknown option %s\n", argv[1]);
    print_help(stderr);
    return CMD_UNKNOWN;
}
