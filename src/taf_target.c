#include "taf_target.h"

#include "internal_logging.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "util/files.h"
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int convert_project_to_multitarget(project_parsed_t *proj) {
    char *dir_path;
    asprintf(&dir_path, "%s/tests/common", proj->project_path);
    int res = create_directory(dir_path, MKDIR_MODE);
    free(dir_path);
    proj->multitarget = true;

    return res;
}

int taf_target_add() {

    cmd_target_add_options *opts = cmd_parser_get_target_add_options();
    if (opts->internal_logging && internal_logging_init()) {
        fprintf(stderr, "Unable to init internal logging.\n");
    }

    LOG("Starting taf target add...");

    if (project_parser_parse()) {
        internal_logging_deinit();
        return EXIT_FAILURE;
    }

    project_parsed_t *proj = get_parsed_project();

    if (!proj->multitarget) {
        LOG("Project is not multitarget.");
    repeat:
        printf(
            "Project is not multitarget, do you want to convert it? [Y/n]\n");
        fflush(stdout);
        int first, ch;
        first = getchar();
        if (first != '\n') {
            do {
                ch = getchar();
            } while (ch != '\n' && ch != EOF);
        }

        if (first == 'N' || first == 'n') {
            LOG("Aborting by user choice.");
            puts("Aborting...\n");
            internal_logging_deinit();
            return EXIT_SUCCESS;
        }
        if (first != 'Y' && first != 'y' && first != '\n') {
            goto repeat;
        }
        LOG("Converting...");
        if (convert_project_to_multitarget(proj)) {
            LOG("Error converting.");
            fputs("Error converting project to multitarget. Check "
                  "permissions.\n",
                  stderr);
            internal_logging_deinit();
            return EXIT_FAILURE;
        }
        printf("Successfully converted project '%s' to multitarget.\n"
               "Your files remained intact, please reorganize them.\n\n",
               proj->project_name);
    }

    for (size_t i = 0; i < proj->targets_amount; i++) {
        LOG("targets[%zu]: %s", i, proj->targets[i]);
        if (!strcmp(proj->targets[i], opts->target)) {
            LOG("Target already exists.");
            fprintf(stderr, "Target '%s' already exists in project.\n",
                    opts->target);
            internal_logging_deinit();
            return EXIT_FAILURE;
        }
    }

    LOG("Adding new target...");

    proj->targets_amount++;
    proj->targets =
        realloc(proj->targets, sizeof(*proj->targets) * proj->targets_amount);
    proj->targets[proj->targets_amount - 1] = strdup(opts->target);

    LOG("Target added.");

    for (size_t i = 0; i < proj->targets_amount; i++) {
        LOG("targets[%zu]: %s", i, proj->targets[i]);
    }

    project_parser_save();

    LOG("Creating target directory...");
    char target_path[PATH_MAX];
    snprintf(target_path, PATH_MAX, "%s/tests/%s", proj->project_path,
             opts->target);
    if (!directory_exists(target_path)) {
        if (create_directory(target_path, MKDIR_MODE)) {
            fprintf(stderr,
                    "Unable to create target directory, unknown error.\n");
            LOG("Unable to create target directory.");
            internal_logging_deinit();
            return EXIT_FAILURE;
        }
    } else {
        LOG("Directory exists.");
    }

    LOG("Successfully added new target.");
    printf("Successfully added new target '%s' to the project '%s'\n",
           opts->target, proj->project_name);

    internal_logging_deinit();

    return EXIT_SUCCESS;
}

int taf_target_remove() {

    cmd_target_remove_options *opts = cmd_parser_get_target_remove_options();
    if (opts->internal_logging && internal_logging_init()) {
        fprintf(stderr, "Unable to init internal logging.\n");
        return EXIT_FAILURE;
    }

    LOG("Starting taf target remove...");

    if (project_parser_parse()) {
        return EXIT_FAILURE;
    }

    project_parsed_t *proj = get_parsed_project();

    if (!proj->multitarget) {
        LOG("Project is not multitarget.");
        fprintf(stderr,
                "Unable to remove target: project is not multitarget.\n");
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < proj->targets_amount; i++) {
        LOG("targets[%zu]: %s", i, proj->targets[i]);
        if (!strcmp(proj->targets[i], opts->target)) {
            LOG("Target for deletion found.");

            for (size_t j = i; j < proj->targets_amount - 1; j++) {
                proj->targets[j] = proj->targets[j + 1];
            }

            proj->targets_amount--;

            for (size_t j = 0; j < proj->targets_amount; j++) {
                LOG("targets[%zu]: %s", j, proj->targets[j]);
            }

            if (proj->targets_amount == 0) {
            repeat:
                printf(
                    "Removing last target '%s'.\n"
                    "Do you wish to convert project to single-target? [Y/n]\n",
                    opts->target);
                fflush(stdout);
                int first, ch;
                first = getchar();
                if (first != '\n') {
                    do {
                        ch = getchar();
                    } while (ch != '\n' && ch != EOF);
                }

                if (first == 'Y' || first == 'y' || first == '\n') {
                    LOG("Converting project to single-target...");
                    proj->multitarget = false;
                    printf("Successfully converted project '%s' to "
                           "single-target.\n",
                           proj->project_name);
                } else if (first != 'N' && first != 'n') {
                    goto repeat;
                }
            }

            project_parser_save();

            LOG("Successfully removed target.");
            printf("Successfully removed target '%s' from project '%s'\n"
                   "\n"
                   "Remove/rename it's test directory if nessessary.\n",
                   opts->target, proj->project_name);

            return EXIT_SUCCESS;
        }
    }

    LOG("Target not found.");
    fprintf(stderr, "Unable to find target '%s' in project '%s'\n",
            opts->target, proj->project_name);

    return EXIT_FAILURE;
}
