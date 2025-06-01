#include "taf_test.h"

#include "cmd_parser.h"
#include "project_parser.h"
#include "taf_tui.h"
#include "test_case.h"
#include "test_logs.h"
#include "version.h"

#include "modules/serial/taf-serial.h"
#include "modules/taf/taf.h"
#include "modules/web/taf-webdriver.h"

#include "util/files.h"
#include "util/time.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int g_first = 0;
static int g_last = 0;

static const char *module_path;

static char test_folder_path[PATH_MAX];
static char lib_folder_path[PATH_MAX];

typedef struct line_cache {
    char *path;
    char **lines;
    struct line_cache *next;
} line_cache_t;

static line_cache_t *cache_head = NULL;

static char *get_line_text(const char *path, int lineno) {

    for (line_cache_t *c = cache_head; c; c = c->next) {
        if (strcmp(c->path, path) == 0) {
            return c->lines[lineno - 1]; // lineno is 1-based
        }
    }

    FILE *fp = fopen(path, "r");
    if (!fp)
        return NULL;

    line_cache_t *c = malloc(sizeof *c);
    c->path = strdup(path);
    c->lines = NULL;
    c->next = cache_head;
    cache_head = c;

    size_t cap = 0, n = 0;
    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fp) > 0) {
        if (n == cap) {
            cap = cap ? cap * 2 : 128;
            c->lines = realloc(c->lines, sizeof(char *) * (cap + 1));
        }
        c->lines[n++] = strdup(line);
    }
    free(line);
    c->lines[n] = NULL;
    fclose(fp);

    return (lineno > 0 && lineno <= (int)n) ? c->lines[lineno - 1] : NULL;
}

static void line_hook(lua_State *L, lua_Debug *ar) {
    if (lua_getinfo(L, "Sl", ar) && ar->currentline > 0) {

        const char *src = ar->source;
        if (src[0] == '@')
            src++;

        if (strncasecmp(module_path, src, strlen(module_path)) == 0) {
            return;
        }

        const char *text = get_line_text(src, ar->currentline);

        taf_tui_set_current_line(ar->short_src, ar->currentline,
                                 text ? text : "(source unavailable)");

        int div = g_last - g_first;
        float percentage;
        percentage =
            div == 0 ? 0 : (float)(ar->currentline - g_first) / div * 100;
        taf_tui_set_test_progress((int)percentage);

        taf_tui_update();
    }
}

static void run_deferred(lua_State *L, const char *status) {
    lua_getfield(L, LUA_REGISTRYINDEX, DEFER_LIST_KEY);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    int list = lua_gettop(L);
    lua_Integer n = luaL_len(L, list);

    for (lua_Integer i = n; i >= 1; --i) { // LIFO
        lua_rawgeti(L, list, i);
        int tbl = lua_gettop(L);

        int argcount = (int)luaL_len(L, tbl) - 1;
        lua_rawgeti(L, tbl, 1);
        for (int a = 2; a <= argcount + 1; ++a)
            lua_rawgeti(L, tbl, a);

        // Push status *only* when user did not supply extra args
        if (argcount == 0) {
            lua_pushstring(L, status);
            argcount += 1;
        }

        if (lua_pcall(L, argcount, 0, 0) != LUA_OK) {
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }

    /* clear list */
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, DEFER_LIST_KEY);
}

static int run_all_tests(lua_State *L) {
    size_t passed = 0;

    size_t amount;
    test_case_t *tests = test_case_get_all(&amount);
    taf_log_tests_create(amount);

    for (size_t i = 0; i < amount; ++i) {

        taf_log_test_started(i + 1, tests[i]);

        lua_rawgeti(L, LUA_REGISTRYINDEX, tests[i].ref);

        lua_pushvalue(L, -1);

        lua_Debug ar;
        if (lua_getinfo(L, ">S", &ar)) {
            g_first = ar.linedefined;
            g_last = ar.lastlinedefined;
        }

        // Reset starting time for millis() function
        reset_millis();

        int rc = lua_pcall(L, 0, 0, 0);

        const char *safe_msg;

        if (rc != LUA_OK) {
            const char *errmsg = lua_tostring(L, -1);
            safe_msg = errmsg ? errmsg : "unknown error";
            lua_pop(L, 1);
        }

        run_deferred(L, (rc == LUA_OK) ? "passed" : "failed");

        if (rc == LUA_OK) {
            taf_log_test_passed(i + 1, tests[i]);
            passed++;
        } else {
            taf_log_test_failed(i + 1, tests[i], safe_msg);
        }
        // Collect some garbage after modules
        // This is mostly done if test has failed
        // But also good if user forgot to close session(s)
        module_web_close_all_sessions();
        module_serial_close_all_ports();
    }

    taf_log_tests_finalize();

    return passed == amount ? EXIT_SUCCESS : EXIT_FAILURE;
}

static const char *get_lib_dir() {

    // TODO: check optional CMD arg first

    // Check 'TAF_LIB_PATH' env variable
    const char *env_path = getenv("TAF_LIB_PATH");
    if (env_path && *env_path) {
        fprintf(stdout, "TAF_LIB_PATH is set.\n");
        fprintf(stdout, "Loading taf lua library at path '%s'\n", env_path);

        return env_path;
    }

    // Check default fallback
    const char *home_path = getenv("HOME");
    if (!home_path || !*home_path) {
        fprintf(stderr, "Unable to load taf lua library: 'HOME' "
                        "environment variable is not set.\n"
                        "Use --libpath path_to_taf_lib_folder or set "
                        "'TAF_LIB_PATH' environment variable.\n");
        exit(EXIT_FAILURE);
    }
    char default_path[PATH_MAX];
    snprintf(default_path, sizeof default_path, "%s/.taf/" TAF_VERSION "/lib",
             home_path);

    return strdup(default_path);
}

static void inject_modules_dir(lua_State *L) {

    module_path = get_lib_dir();

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path"); /* pkg.path string */
    const char *old_path = lua_tostring(L, -1);
    lua_pushfstring(L,
                    "%s;%s/?.lua;%s/?/init.lua;%s/?.lua;%s/?/init.lua;%s/"
                    "?.lua;%s/?/init.lua",
                    old_path, module_path, module_path, lib_folder_path,
                    lib_folder_path, test_folder_path, test_folder_path);
    lua_setfield(L, -3, "path"); /* package.path = … */
    lua_pop(L, 2);               /* pop path + package */
}

static void register_test_api(lua_State *L) {

    // Change default lua 'print' to our implementation:
    lua_pushcfunction(L, l_module_taf_print);
    lua_setglobal(L, "print");

    // Register C lua modules:
    luaL_requiref(L, "taf-serial", l_module_serial_register_module, 1);
    lua_pop(L, 1);
    luaL_requiref(L, "taf-main", l_module_taf_register_module, 1);
    lua_pop(L, 1);
    luaL_requiref(L, "taf-webdriver", l_module_web_register_module, 1);
    lua_pop(L, 1);

    inject_modules_dir(L);
}

static int load_lua_files(lua_State *L, str_array_t *files) {

    for (size_t i = 0; i < files->count; i++) {
        if (luaL_dofile(L, files->items[i])) {
            fprintf(stderr, "Lua error loading %s: %s\n", files->items[i],
                    lua_tostring(L, -1));
            lua_pop(L, 1);
            return -1;
        }
    }

    return 0;
}

int taf_test() {

    if (project_parser_parse()) {
        // Already handled
        return EXIT_FAILURE;
    }

    project_parsed_t *proj = get_parsed_project();

    cmd_test_options *opts = cmd_parser_get_test_options();

    if (!opts->target && proj->multitarget) {
        fprintf(stderr, "Project is multitarget, specify target with "
                        "'--target' option.\n");
        return EXIT_FAILURE;
    }
    if (opts->target && !proj->multitarget) {
        fprintf(stderr, "Unknown target '%s', project is not multitarget.\n",
                opts->target);
        return EXIT_FAILURE;
    }

    // Load test paths
    if (proj->multitarget) {
        snprintf(test_folder_path, PATH_MAX, "%s/tests/%s", proj->project_path,
                 opts->target);
        if (!directory_exists(test_folder_path)) {
            fprintf(stderr, "Unable to find target %s.\n", opts->target);
            return EXIT_FAILURE;
        }
    } else {
        snprintf(test_folder_path, PATH_MAX, "%s/tests", proj->project_path);
        if (!directory_exists(test_folder_path)) {
            fprintf(stderr, "Unable to find 'tests' folder.\n");
            return EXIT_FAILURE;
        }
    }
    snprintf(lib_folder_path, PATH_MAX, "%s/lib", proj->project_path);

    str_array_t lua_test_files = list_lua_recursive(test_folder_path);
    if (lua_test_files.count == 0) {
        fprintf(stderr, "No tests to execute.\n");
        return EXIT_FAILURE;
    }

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    register_test_api(L);

    // Load lib files if they are present
    char lib_folder_path[PATH_MAX];
    snprintf(lib_folder_path, PATH_MAX, "%s/lib", proj->project_path);
    if (directory_exists(lib_folder_path)) {

        str_array_t lua_lib_files = list_lua_recursive(lib_folder_path);

        if (load_lua_files(L, &lua_lib_files))
            return EXIT_FAILURE;

        free_str_array(&lua_lib_files);
    }

    if (load_lua_files(L, &lua_test_files))
        return EXIT_FAILURE;

    free_str_array(&lua_test_files);

    size_t amount;
    test_case_get_all(&amount);

    if (amount == 0) {
        fprintf(stderr, "No tests to execute.\n");
        return EXIT_FAILURE;
    }

    if (taf_tui_init())
        return EXIT_FAILURE;

    lua_sethook(L, line_hook, LUA_MASKLINE, 0); // Enable line hook

    /* -------- run the queued tests ------------------ */
    int exitcode = run_all_tests(L);

    /* -------- tidy‑up ------------------------------- */
    taf_tui_deinit();

    test_case_free_all(L);

    lua_close(L);

    return exitcode;
}
