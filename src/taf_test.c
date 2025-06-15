#include "taf_test.h"

#include "internal_logging.h"

#include "cmd_parser.h"
#include "modules/http/taf-http.h"
#include "project_parser.h"
#include "taf_tui.h"
#include "test_case.h"
#include "test_logs.h"
#include "version.h"

#include "modules/json/taf-json.h"
#include "modules/proc/taf-proc.h"
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

static char *module_path;

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
    LOG("Running deferred test queue...");

    lua_getfield(L, LUA_REGISTRYINDEX, DEFER_LIST_KEY);
    if (lua_isnil(L, -1)) {
        LOG("Defer queue is empty.");
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
            LOG("Argcount is 0, pushing status '%s'...", status);
            lua_pushstring(L, status);
            argcount += 1;
        }

        LOG("Executing defer %lld with argcount %d...", i, argcount);
        int rc = lua_pcall(L, argcount, 0, 0);
        LOG("Executed defer %lld, status: %d", i, rc);
        lua_pop(L, 1);
    }

    LOG("Clearing defer list...");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, DEFER_LIST_KEY);
    LOG("Defer list cleared.");

    LOG("Finished running defer queue.");
}

static int taf_errhandler(lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    if (!msg)
        msg = "(non-string error)";
    luaL_traceback(L, L, msg, 1); // replaces TOS with traceback
    return 1;                     // 1 return value for pcall
}

static int run_all_tests(lua_State *L) {
    LOG("Running tests...");

    size_t passed = 0;

    size_t amount;
    test_case_t *tests = test_case_get_all(&amount);
    LOG("Test amount: %zu", amount);
    taf_log_tests_create(amount);

    for (size_t i = 0; i < amount; ++i) {

        taf_log_test_started(i + 1, tests[i]);

        LOG("Setting up error handler...");
        lua_pushcfunction(L, taf_errhandler);
        int erridx = lua_gettop(L);
        LOG("Error handler index: %d", erridx);

        LOG("Pushing test body with index %d...", tests[i].ref);
        lua_rawgeti(L, LUA_REGISTRYINDEX, tests[i].ref);

        LOG("Resetting taf.millis...");
        reset_millis();

        LOG("Executing test '%s'...", tests[i].name);
        int rc = lua_pcall(L, 0, 0, erridx);
        LOG("Finished executing test '%s', status: %d", tests[i].name, rc);

        char *file = NULL;
        int line = 0;
        char *trace = NULL;

        if (rc != LUA_OK) {
            trace = strdup(lua_tostring(L, -1)); /* traceback string */
            LOG("Test '%s' traceback: %s", tests[i].name, trace);

            /* first line looks like  "<file>:<line>: <message>" */
            if (trace) {
                const char *colon1 = strchr(trace, ':');
                if (colon1) {
                    const char *colon2 = strchr(colon1 + 1, ':');
                    if (colon2) {
                        file = strndup(trace, colon1 - trace);
                        line = atoi(colon1 + 1);
                    }
                }
            }
            lua_pop(L, 1); /* pop traceback */
        }

        LOG("Popping error handler...");
        lua_remove(L, erridx);

        run_deferred(L, rc == LUA_OK ? "passed" : "failed");

        if (rc == LUA_OK) {
            taf_log_test_passed(i + 1, tests[i]);
            passed++;
        } else {
            taf_log_test_failed(i + 1, tests[i],
                                trace ? trace : "unknown error",
                                file ? file : "(?)", line);
            free(file);
            free(trace);
        }
    }

    taf_log_tests_finalize();

    return passed == amount ? EXIT_SUCCESS : EXIT_FAILURE;
}

static char *get_lib_dir() {
    LOG("Getting TAF library directory location...");

    // TODO: check optional CMD arg first

    // Check 'TAF_LIB_PATH' env variable
    char *env_path = getenv("TAF_LIB_PATH");
    if (env_path && *env_path) {
        LOG("TAF_LIB_PATH specified: %s", env_path);
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
        LOG("Unable to find TAF library directory: HOME is not set.");
        internal_logging_deinit();
        exit(EXIT_FAILURE);
    }
    LOG("HOME path: %s", home_path);
    char default_path[PATH_MAX];
    snprintf(default_path, sizeof default_path, "%s/.taf/" TAF_VERSION "/lib",
             home_path);
    LOG("TAF library path: %s", default_path);

    return strdup(default_path);
}

static void inject_modules_dir(lua_State *L) {
    LOG("Injecting TAF library directory...");

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

    LOG("Successfully injected TAF library directory.");

}

static void register_clua_module(lua_State *L, const char *name,
                                 lua_CFunction openf) {
    luaL_requiref(L, name, openf, 1);
    lua_pop(L, 1);
}

static void register_test_api(lua_State *L) {

    LOG("Registering test API...");

    // Change default lua 'print' to our implementation:
    lua_pushcfunction(L, l_module_taf_print);
    lua_setglobal(L, "print");

    // Register C lua modules:
    register_clua_module(L, "taf-http", l_module_http_register_module);
    register_clua_module(L, "taf-json", l_module_json_register_module);
    register_clua_module(L, "taf-main", l_module_taf_register_module);
    register_clua_module(L, "taf-proc", l_module_proc_register_module);
    register_clua_module(L, "taf-serial", l_module_serial_register_module);
    register_clua_module(L, "taf-webdriver", l_module_web_register_module);

    inject_modules_dir(L);

    LOG("Test API registered.");
}

static int load_lua_files(lua_State *L, str_array_t *files) {

    for (size_t i = 0; i < files->count; i++) {
        char *file = files->items[i];
        LOG("Loading Lua file %s...", file);
        if (luaL_dofile(L, file)) {
            const char *err = lua_tostring(L, -1);
            LOG("Failed loading: %s", err);
            fprintf(stderr, "Lua error loading %s: %s\n", file, err);
            lua_pop(L, 1);
            return -1;
        }
        LOG("File %s loaded successfully.", file);
    }

    return 0;
}

int taf_test() {

    cmd_test_options *opts = cmd_parser_get_test_options();

    if (opts->internal_logging && internal_logging_init()) {
        fprintf(stderr, "Unable to init internal logging.\n");
        return EXIT_FAILURE;
    }

    LOG("Starting TAF testing...");

    if (project_parser_parse()) {
        // Already handled
        internal_logging_deinit();
        return EXIT_FAILURE;
    }

    project_parsed_t *proj = get_parsed_project();

    if (!opts->target && proj->multitarget) {
        fprintf(stderr, "Project is multitarget, specify target with "
                        "'--target' option.\n");
        LOG("No target specified.");
        internal_logging_deinit();
        return EXIT_FAILURE;
    }
    if (opts->target && !proj->multitarget) {
        fprintf(stderr, "Unknown target '%s', project is not multitarget.\n",
                opts->target);
        LOG("Target specified, but project is not multitarget.");
        internal_logging_deinit();
        return EXIT_FAILURE;
    }

    // Load test paths
    if (proj->multitarget) {
        LOG("Target: %s", opts->target);
        snprintf(test_folder_path, PATH_MAX, "%s/tests/%s", proj->project_path,
                 opts->target);
        LOG("Test folder path: %s", test_folder_path);
        if (!directory_exists(test_folder_path)) {
            fprintf(stderr, "Unable to find 'tests/%s' folder.\n",
                    opts->target);
            LOG("Test folder path does not exist.");
            internal_logging_deinit();
            return EXIT_FAILURE;
        }
    } else {
        snprintf(test_folder_path, PATH_MAX, "%s/tests", proj->project_path);
        if (!directory_exists(test_folder_path)) {
            LOG("Test folder path does not exist");
            fprintf(stderr, "Unable to find 'tests' folder.\n");
            internal_logging_deinit();
            return EXIT_FAILURE;
        }
    }
    snprintf(lib_folder_path, PATH_MAX, "%s/lib", proj->project_path);
    LOG("Project lib folder path: %s", lib_folder_path);

    str_array_t lua_test_files = list_lua_recursive(test_folder_path);
    if (lua_test_files.count == 0) {
        fprintf(stderr, "No tests to execute.\n");
        LOG("No tests files found.");
        internal_logging_deinit();
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < lua_test_files.count; i++)
        LOG("Found test file %s", lua_test_files.items[i]);

    LOG("Creating Lua state...");
    lua_State *L = luaL_newstate();
    LOG("Opening Lua libs...");
    luaL_openlibs(L);

    register_test_api(L);

    // Load lib files if they are present
    if (directory_exists(lib_folder_path)) {
        LOG("Checking project lib folder...");

        str_array_t lua_lib_files = list_lua_recursive(lib_folder_path);

        if (load_lua_files(L, &lua_lib_files)) {
            free(module_path);
            free_str_array(&lua_lib_files);
            internal_logging_deinit();
            return EXIT_FAILURE;
        }

        free_str_array(&lua_lib_files);
    }

    LOG("Loading test files...");
    if (load_lua_files(L, &lua_test_files))
        return EXIT_FAILURE;

    free_str_array(&lua_test_files);

    size_t amount;
    test_case_get_all(&amount);

    if (amount == 0) {
        LOG("No tests found.");
        fprintf(stderr, "No tests to execute.\n");
        free(module_path);
        internal_logging_deinit();
        return EXIT_FAILURE;
    }

    if (taf_tui_init(opts->log_level)) {
        free(module_path);
        internal_logging_deinit();
        return EXIT_FAILURE;
    }

    LOG("Enabling line hook...");
    lua_sethook(L, line_hook, LUA_MASKLINE, 0); // Enable line hook

    /* -------- run the queued tests ------------------ */
    int exitcode = run_all_tests(L);

    /* -------- tidy‑up ------------------------------- */
    LOG("Tidying up...");

    taf_tui_deinit();

    test_case_free_all(L);

    LOG("Closing Lua state...");
    lua_close(L);

    project_parser_free();

    internal_logging_deinit();

    free(module_path);

    return exitcode;
}
