#include "taf_init.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "version.h"

#include "util/files.h"

#include <stdio.h>
#include <stdlib.h>
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
    if (create_directory(opts->project_name, MKDIR_MODE))
        return -1;

    char path[PATH_MAX];

    snprintf(path, PATH_MAX, "%s/lib", opts->project_name);
    if (create_directory(path, MKDIR_MODE))
        return -2;

    snprintf(path, PATH_MAX, "%s/tests", opts->project_name);
    if (create_directory(path, MKDIR_MODE))
        return -3;

    if (opts->multitarget) {
        snprintf(path, PATH_MAX, "%s/tests/common", opts->project_name);
        if (create_directory(path, MKDIR_MODE))
            return -4;
    }

    project_parsed_t *proj = get_parsed_project();
    proj->project_path = opts->project_name;
    proj->project_name = (char *)opts->project_name;
    proj->multitarget = opts->multitarget;
    proj->min_taf_ver_str = TAF_VERSION;
    proj->min_taf_ver_major = TAF_VERSION_MAJOR;
    proj->min_taf_ver_minor = TAF_VERSION_MINOR;
    proj->min_taf_ver_patch = TAF_VERSION_PATCH;

    project_parser_save();

    char gitignore_path[PATH_MAX];
    snprintf(gitignore_path, PATH_MAX, "%s/.gitignore", proj->project_path);
    FILE *fp = fopen(gitignore_path, "w");
    if (!fp) {
        perror("fopen");
        return -5;
    }

    fprintf(fp, "%s", gitignore_contents);
    fflush(fp);
    fclose(fp);

    return 0;
}

int taf_init() {

    const char *proj_file = file_find_upwards(".taf.json");
    if (proj_file && *proj_file) {
        fprintf(
            stderr,
            "Already in the project, use 'taf config' instead to tweak it.\n");
        return EXIT_FAILURE;
    }

    cmd_init_options *opts = cmd_parser_get_init_options();
    if (directory_exists(opts->project_name)) {
        fprintf(stderr, "Project with name %s already exists.\n",
                opts->project_name);
        return EXIT_FAILURE;
    }

    int ret = create_project(opts);
    if (ret) {
        fprintf(stderr, "Unable to create project, error code: %d\n", ret);
        return EXIT_FAILURE;
    }

    printf("Created new project '%s'\n", opts->project_name);

    return EXIT_SUCCESS;
}
