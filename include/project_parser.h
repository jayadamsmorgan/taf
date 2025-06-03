#ifndef PROJECT_PARSER_H
#define PROJECT_PARSER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *project_name;
    char *min_taf_ver_str;
    int min_taf_ver_major;
    int min_taf_ver_minor;
    int min_taf_ver_patch;

    const char *project_path;

    bool multitarget;

    char **targets;
    size_t targets_amount;
    // TODO: more

} project_parsed_t;

bool project_parser_parse();

project_parsed_t *get_parsed_project();

void project_parser_save();

#endif // PROJECT_PARSER_H
