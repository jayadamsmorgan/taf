#include "project_parser.h"

#include "internal_logging.h"

#include "util/files.h"

#include <json.h>

#include <libgen.h>
#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
#include <sys/syslimits.h>
#else
#include <limits.h>
#endif // __APPLE__

static project_parsed_t *proj_parsed = NULL;

project_parsed_t *get_parsed_project() {
    //
    return proj_parsed;
}

static char *json_get_string(json_object *obj, const char *key) {
    json_object *sub = json_object_object_get(obj, key);
    if (!sub || !json_object_is_type(sub, json_type_string)) {
        return NULL;
    }
    return (char *)json_object_get_string(sub);
}

static bool json_get_bool(json_object *obj, const char *key, bool *res) {
    json_object *sub = json_object_object_get(obj, key);
    if (!sub || !json_object_is_type(sub, json_type_boolean)) {
        return true;
    }
    if (res) {
        *res = json_object_get_boolean(sub);
    }
    return false;
}

static bool json_get_int(json_object *obj, const char *key, int *res) {
    json_object *sub = json_object_object_get(obj, key);
    if (!sub || !json_object_is_type(sub, json_type_int)) {
        return true;
    }
    if (res) {
        *res = json_object_get_int(sub);
    }
    return false;
}

static int project_parse_json(json_object *project_obj) {

    if (!json_object_is_type(project_obj, json_type_object)) {
        LOG("Not a json object.");
        return -1;
    }

    char *str_res = json_get_string(project_obj, "project_name");
    if (!str_res) {
        LOG("Project name not found.");
        return -1;
    }
    proj_parsed->project_name = strdup(str_res);
    LOG("Project.project_name: %s", proj_parsed->project_name);

    str_res = json_get_string(project_obj, "min_taf_version");
    if (!str_res) {
        LOG("Project mimimum version not found.");
        return -1;
    }
    proj_parsed->min_taf_ver_str = strdup(str_res);
    LOG("Project.min_taf_version: %s", proj_parsed->min_taf_ver_str);

    if (json_get_int(project_obj, "min_taf_version_major",
                     &proj_parsed->min_taf_ver_major)) {
        LOG("Project minimum major version not found.");
        return -1;
    }
    LOG("Projcet.min_taf_ver_major: %d", proj_parsed->min_taf_ver_major);
    if (json_get_int(project_obj, "min_taf_version_minor",
                     &proj_parsed->min_taf_ver_minor)) {
        LOG("Project minimum minor version not found.");
        return -1;
    }
    LOG("Project.min_taf_ver_minor: %d", proj_parsed->min_taf_ver_minor);
    if (json_get_int(project_obj, "min_taf_version_patch",
                     &proj_parsed->min_taf_ver_patch)) {
        LOG("Project minimum patch version not found.");
        return -1;
    }
    LOG("Project.min_taf_ver_patch: %d", proj_parsed->min_taf_ver_patch);

    if (json_get_bool(project_obj, "multitarget", &proj_parsed->multitarget)) {
        LOG("Project multitarget field not found.");
        return -1;
    }
    LOG("Project.multitarget: %d", proj_parsed->multitarget);

    if (proj_parsed->multitarget) {
        json_object *target_arr;
        if (!json_object_object_get_ex(project_obj, "targets", &target_arr)) {
            LOG("Project is multitarget, but no targets found.");
            return -1;
        }
        if (!json_object_is_type(target_arr, json_type_array)) {
            LOG("Project targets is not an array.");
            return -1;
        }
        proj_parsed->targets_amount = json_object_array_length(target_arr);
        if (proj_parsed->targets_amount != 0) {
            proj_parsed->targets = malloc(sizeof(*proj_parsed->targets) *
                                          proj_parsed->targets_amount);
        }
        for (size_t i = 0; i < proj_parsed->targets_amount; i++) {
            json_object *target_obj = json_object_array_get_idx(target_arr, i);
            proj_parsed->targets[i] =
                strdup(json_object_get_string(target_obj));
            LOG("Project.targets[%zu]: %s", i, proj_parsed->targets[i]);
        }
    }

    return 0;
}

void project_parser_save() {

    LOG("Saving project file...");

    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "project_name",
                           json_object_new_string(proj_parsed->project_name));
    json_object_object_add(
        obj, "min_taf_version",
        json_object_new_string(proj_parsed->min_taf_ver_str));
    json_object_object_add(obj, "min_taf_version_major",
                           json_object_new_int(proj_parsed->min_taf_ver_major));
    json_object_object_add(obj, "min_taf_version_minor",
                           json_object_new_int(proj_parsed->min_taf_ver_minor));
    json_object_object_add(obj, "min_taf_version_patch",
                           json_object_new_int(proj_parsed->min_taf_ver_patch));

    json_object_object_add(obj, "multitarget",
                           json_object_new_boolean(proj_parsed->multitarget));

    if (proj_parsed->multitarget) {
        json_object *target_arr = json_object_new_array();
        for (size_t i = 0; i < proj_parsed->targets_amount; i++) {
            json_object_array_add(
                target_arr, json_object_new_string(proj_parsed->targets[i]));
        }
        json_object_object_add(obj, "targets", target_arr);
    }

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/.taf.json", proj_parsed->project_path);
    if (json_object_to_file_ext(path, obj,
                                JSON_C_TO_STRING_SPACED |
                                    JSON_C_TO_STRING_PRETTY |
                                    JSON_C_TO_STRING_NOSLASHESCAPE)) {
        const char *err = json_util_get_last_err();
        LOG("Unable to save project file: %s", err);
        fprintf(stderr,
                "Unknown error occured, unable to save project file: %s\n",
                err);
        internal_logging_deinit();
        exit(EXIT_FAILURE);
    }

    LOG("Project file saved.");
}

bool project_parser_parse() {

    LOG("Parsing project file...");

    proj_parsed = malloc(sizeof *proj_parsed);

    const char *project_file = file_find_upwards(".taf.json");
    if (!project_file) {
        LOG("Project file not found.");
        fprintf(stderr,
                "Unable to find project file. Make sure you are in project.\n");
        return true;
    }
    LOG("Project file found: %s", project_file);

    json_object *project_obj = json_object_from_file(project_file);
    if (!project_obj) {
        const char *err = json_util_get_last_err();
        const char *err_text = "Unable to get json object from project file: ";
        LOG("%s%s.", err_text, err);
        fprintf(stderr, "%s%s\n", err_text, err);
        return true;
    }

    char *tmp = strdup(project_file);
    proj_parsed->project_path = strdup(dirname(tmp));
    free(tmp);

    LOG("Project.project_path: %s", proj_parsed->project_path);

    if (project_parse_json(project_obj)) {
        LOG("Unable to parse project file.");
        fprintf(
            stderr,
            "Unable to parse project file, it may be corrupt.\n "
            "Please delete it and regenerate the project with 'taf init'\n");
        return true;
    }

    json_object_put(project_obj);

    LOG("Project file parsed successfully.");

    return false;
}

void project_parser_free() {
    LOG("Freeing parsed project...");
    free(proj_parsed->project_path);
    free(proj_parsed->project_name);
    for (size_t i = 0; i < proj_parsed->targets_amount; i++)
        free(proj_parsed->targets[i]);
    free(proj_parsed->targets);
    free(proj_parsed);
    LOG("Parsed project freed.");
}
