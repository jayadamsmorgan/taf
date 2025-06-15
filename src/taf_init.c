#include "taf_init.h"

#include "internal_logging.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "version.h"

#include "util/files.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <sys/syslimits.h>
#else
#include <limits.h>
#endif // __APPLE__

static const char *gitignore_contents = //
    "logs/\n"                           //
    "\n"                                //
    "\n"                                //
    ".DS_Store"                         //
    ;

static int create_project(cmd_init_options *opts) {
    LOG("Creating project...");
    LOG("Project name: %s", opts->project_name);
    LOG("Multitarget: %d", opts->multitarget);

    LOG("Creating project directory '%s'...", opts->project_name);
    if (create_directory(opts->project_name, MKDIR_MODE)) {
        LOG("Unable to create project directory.");
        return -1;
    }

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/lib", opts->project_name);

    LOG("Creating lib directory '%s'...", path);
    if (create_directory(path, MKDIR_MODE)) {
        LOG("Unable to create lib directory.");
        return -2;
    }

    snprintf(path, PATH_MAX, "%s/tests", opts->project_name);

    LOG("Creating tests directory '%s'...", path);
    if (create_directory(path, MKDIR_MODE)) {
        LOG("Unable to create tests directory.");
        return -3;
    }

    if (opts->multitarget) {
        snprintf(path, PATH_MAX, "%s/tests/common", opts->project_name);
        LOG("Creating tests/common directory '%s'...", path);
        if (create_directory(path, MKDIR_MODE)) {
            LOG("Unable to create common directory.");
            return -4;
        }
    }

    LOG("Creating project object...");
    parsed_project_init();
    project_parsed_t *proj = get_parsed_project();
    proj->project_path = strdup(opts->project_name);
    proj->project_name = strdup(opts->project_name);
    proj->multitarget = opts->multitarget;
    proj->min_taf_ver_str = strdup(TAF_VERSION);
    proj->min_taf_ver_major = TAF_VERSION_MAJOR;
    proj->min_taf_ver_minor = TAF_VERSION_MINOR;
    proj->min_taf_ver_patch = TAF_VERSION_PATCH;

    project_parser_save();

    char gitignore_path[PATH_MAX];
    snprintf(gitignore_path, PATH_MAX, "%s/.gitignore", proj->project_path);
    LOG("Creating .gitignore file '%s'", gitignore_path);
    FILE *fp = fopen(gitignore_path, "w");
    if (!fp) {
        LOG("Unable to create .gitignore file.");
        return -5;
    }

    LOG("Writing .gitignore contents...");
    fprintf(fp, "%s", gitignore_contents);
    LOG("Closing & flushing .gitignore file...");
    fflush(fp);
    fclose(fp);

    LOG("Successfully finished creating project.");

    return 0;
}

int taf_init() {

    cmd_init_options *opts = cmd_parser_get_init_options();

    if (opts->internal_logging && internal_logging_init()) {
        fprintf(stderr, "Unable to init internal logging.\n");
        return EXIT_FAILURE;
    }

    LOG("Starting taf init...");

    const char *proj_file = file_find_upwards(".taf.json");
    if (proj_file && *proj_file) {
        LOG("Project already exists.");
        fprintf(
            stderr,
            "Already in the project, use 'taf config' instead to tweak it.\n");
        internal_logging_deinit();
        return EXIT_FAILURE;
    }

    if (directory_exists(opts->project_name)) {
        LOG("Project already exists.");
        fprintf(stderr, "Project with name %s already exists.\n",
                opts->project_name);
        internal_logging_deinit();
        return EXIT_FAILURE;
    }

    int ret = create_project(opts);
    if (ret) {
        fprintf(stderr, "Unable to create project, error code: %d\n", ret);
        internal_logging_deinit();
        return EXIT_FAILURE;
    }

    printf("Created new project '%s'\n", opts->project_name);

    project_parser_free();

    internal_logging_deinit();

    return EXIT_SUCCESS;
}
