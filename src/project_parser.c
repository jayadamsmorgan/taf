#include "project_parser.h"

#include "util/files.h"

#include <json.h>

#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/syslimits.h>

static project_parsed_t proj_parsed = {0};

project_parsed_t *get_parsed_project() {
    //
    return &proj_parsed;
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
        return -1;
    }

    if (proj_parsed.project_name)
        free(proj_parsed.project_name);
    char *str_res = json_get_string(project_obj, "project_name");
    if (!str_res)
        return -1;
    proj_parsed.project_name = strdup(str_res);

    if (proj_parsed.min_taf_ver_str)
        free(proj_parsed.min_taf_ver_str);
    str_res = json_get_string(project_obj, "min_taf_version");
    if (!str_res)
        return -1;
    proj_parsed.min_taf_ver_str = strdup(str_res);

    if (json_get_int(project_obj, "min_taf_version_major",
                     &proj_parsed.min_taf_ver_major))
        return -1;
    if (json_get_int(project_obj, "min_taf_version_minor",
                     &proj_parsed.min_taf_ver_minor))
        return -1;
    if (json_get_int(project_obj, "min_taf_version_patch",
                     &proj_parsed.min_taf_ver_patch))
        return -1;

    if (json_get_bool(project_obj, "multitarget", &proj_parsed.multitarget))
        return -1;

    return 0;
}

void project_parser_save() {

    if (!proj_parsed.project_path) {
        fprintf(stderr,
                "Unknown error occured, unable to find project location.\n");
        exit(EXIT_FAILURE);
    }

    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "project_name",
                           json_object_new_string(proj_parsed.project_name));
    json_object_object_add(obj, "min_taf_version",
                           json_object_new_string(proj_parsed.min_taf_ver_str));
    json_object_object_add(obj, "min_taf_version_major",
                           json_object_new_int(proj_parsed.min_taf_ver_major));
    json_object_object_add(obj, "min_taf_version_minor",
                           json_object_new_int(proj_parsed.min_taf_ver_minor));
    json_object_object_add(obj, "min_taf_version_patch",
                           json_object_new_int(proj_parsed.min_taf_ver_patch));

    json_object_object_add(obj, "multitarget",
                           json_object_new_boolean(proj_parsed.multitarget));

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/.taf.json", proj_parsed.project_path);
    if (json_object_to_file(path, obj)) {
        fprintf(stderr,
                "Unknown error occured, unable to save project file: %s\n",
                json_util_get_last_err());
        exit(EXIT_FAILURE);
    }
}

bool project_parser_parse() {

    const char *project_file = file_find_upwards(".taf.json");
    if (!project_file) {
        fprintf(stderr,
                "Unable to find project file. Make sure you are in project.\n");
        return true;
    }

    json_object *project_obj = json_object_from_file(project_file);

    char *tmp = strdup(project_file);
    proj_parsed.project_path = dirname(tmp);
    free(tmp);
    if (project_parse_json(project_obj)) {
        fprintf(
            stderr,
            "Unable to parse project file, it may be corrupt.\n "
            "Please delete it and regenerate the project with 'taf init'\n");
        return true;
    }

    return false;
}
