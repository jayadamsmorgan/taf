#include "taf_target.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "util/files.h"

#include <stdlib.h>
#include <string.h>

int taf_target_add() {

    if (project_parser_parse()) {
        return EXIT_FAILURE;
    }

    project_parsed_t *proj = get_parsed_project();

    if (!proj->multitarget) {
        fprintf(stderr, "Unable to add target: project is not multitarget.\n");
        return EXIT_FAILURE;
    }

    cmd_target_add_options *opts = cmd_parser_get_target_add_options();

    for (size_t i = 0; i < proj->targets_amount; i++) {
        if (!strcmp(proj->targets[i], opts->target)) {
            fprintf(stderr, "Target '%s' already exists in project.\n",
                    opts->target);
            return EXIT_FAILURE;
        }
    }

    proj->targets_amount++;
    proj->targets =
        realloc(proj->targets, sizeof(*proj->targets) * proj->targets_amount);
    proj->targets[proj->targets_amount - 1] = strdup(opts->target);

    project_parser_save();

    char target_path[PATH_MAX];
    snprintf(target_path, PATH_MAX, "%s/tests/%s", proj->project_path,
             opts->target);
    if (!directory_exists(target_path)) {
        create_directory(target_path, MKDIR_MODE);
    }

    printf("Successfully added new target '%s' to the project '%s'\n",
           opts->target, proj->project_name);

    return EXIT_SUCCESS;
}

int taf_target_remove() {

    if (project_parser_parse()) {
        return EXIT_FAILURE;
    }

    project_parsed_t *proj = get_parsed_project();

    if (!proj->multitarget) {
        fprintf(stderr, "Unable to add target: project is not multitarget.\n");
        return EXIT_FAILURE;
    }

    cmd_target_remove_options *opts = cmd_parser_get_target_remove_options();

    for (size_t i = 0; i < proj->targets_amount; i++) {
        if (!strcmp(proj->targets[i], opts->target)) {

            for (size_t j = i; j < proj->targets_amount - 1; j++) {
                proj->targets[j] = proj->targets[j + 1];
            }

            proj->targets_amount--;

            project_parser_save();

            printf("Successfully removed target '%s' from project '%s'\n"
                   "\n"
                   "Remove/rename it's test directory if nessessary.\n",
                   opts->target, proj->project_name);

            return EXIT_SUCCESS;
        }
    }

    fprintf(stderr, "Unable to find target '%s' in project '%s'\n",
            opts->target, proj->project_name);

    return EXIT_FAILURE;
}
