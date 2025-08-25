#include "taf_test.h"

#include "cmd_parser.h"
#include "headless.h"
#include "internal_logging.h"
#include "modules/hooks/taf-hooks.h"
#include "project_parser.h"
#include "taf_hooks.h"
#include "taf_secrets.h"
#include "taf_tui.h"
#include "taf_vars.h"
#include "test_case.h"
#include "test_logs.h"
#include "version.h"

#include "modules/http/taf-http.h"
#include "modules/json/taf-json.h"
#include "modules/proc/taf-proc.h"
#include "modules/serial/taf-serial.h"
#include "modules/taf/taf.h"

#include "util/files.h"
#include "util/line_cache.h"
#include "util/string.h"
#include "util/time.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int g_first = 0;
static int g_last = 0;

static char *module_path = NULL;
static char *test_dir_path = NULL;
static char *test_common_dir_path = NULL;
static char *lib_dir_path = NULL;
static char *hooks_dir_path = NULL;

static bool test_marked_failed = false;
static size_t current_test_index = 0;

void taf_mark_test_failed() { test_marked_failed = true; }

static void line_hook(lua_State *L, lua_Debug *ar) {
    if (lua_getinfo(L, "Sl", ar) && ar->currentline > 0) {

        const char *src = ar->source;
        if (src[0] == '@')
            src++;

        if (string_has_prefix(src, module_path) ||
            string_has_prefix(src, hooks_dir_path)) {
            // Skip internal module lines and hook lines
            return;
        }

        const char *text = get_line_text(src, ar->currentline);

        taf_tui_set_current_line(src, ar->currentline,
                                 text ? text : "(source unavailable)");

        if (string_has_prefix(src, test_dir_path) ||
            string_has_prefix(src, test_common_dir_path)) {
            int div = g_last - g_first;
            double progress;
            progress = div == 0 ? 0 : (double)(ar->currentline - g_first) / div;
            taf_tui_set_test_progress(progress);
        }
    }
}

static int taf_errhandler(lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    if (!msg)
        msg = "(non-string error)";
    luaL_traceback(L, L, msg, 1); // replaces TOS with traceback
    return 1;                     // 1 return value for pcall
}

static void run_deferred(lua_State *L, taf_state_t *state, const char *status) {
    LOG("Running deferred test queue...");

    lua_getfield(L, LUA_REGISTRYINDEX, DEFER_LIST_KEY);
    if (lua_isnil(L, -1)) {
        LOG("Defer queue is empty.");
        lua_pop(L, 1);
        return;
    }

    int list = lua_gettop(L);
    lua_Integer n = luaL_len(L, list);

    taf_state_test_defer_queue_started(state);

    for (lua_Integer i = n; i >= 1; --i) { // LIFO

        LOG("Setting up error handler...");
        lua_pushcfunction(L, taf_errhandler);
        int erridx = lua_gettop(L);
        LOG("Error handler index: %d", erridx);

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
        int rc = lua_pcall(L, argcount, 0, erridx);
        LOG("Executed defer %lld, status: %d", i, rc);
        if (rc != LUA_OK) {
            char *file = NULL;
            int line = 0;
            char *trace = NULL;

            trace = strdup(lua_tostring(L, -1)); /* traceback string */
            LOG("Defer traceback: %s", trace);

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

            taf_state_test_defer_failed(state, file, line, trace);
        }
        lua_pop(L, 1);
    }

    taf_state_test_defer_queue_finished(state);

    LOG("Clearing defer list...");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, DEFER_LIST_KEY);
    LOG("Defer list cleared.");

    LOG("Finished running defer queue.");
}

static int run_all_tests(lua_State *L, taf_state_t *state) {
    LOG("Running tests...");

    size_t passed = 0;

    da_t *tests = test_case_get_all();
    size_t amount = da_size(tests);
    LOG("Test amount: %zu", amount);

    taf_state_test_run_started(state);

    taf_hooks_run(L, TAF_HOOK_FN_TEST_RUN_STARTED);

    reset_taf_start_millis();

    for (size_t i = 0; i < amount; ++i) {

        test_case_t *tc = da_get(tests, i);

        test_marked_failed = false;
        current_test_index = i;

        taf_state_test_started(state, tc);

        taf_hooks_run(L, TAF_HOOK_FN_TEST_STARTED);

        LOG("Setting up error handler...");
        lua_pushcfunction(L, taf_errhandler);
        int erridx = lua_gettop(L);
        LOG("Error handler index: %d", erridx);

        LOG("Pushing test body with index %d...", tc->ref);
        lua_rawgeti(L, LUA_REGISTRYINDEX, tc->ref);
        lua_pushvalue(L, -1);
        lua_Debug ar;
        if (lua_getinfo(L, ">S", &ar)) {
            g_first = ar.linedefined;
            g_last = ar.lastlinedefined;
        }

        LOG("Resetting taf.millis...");
        reset_millis();

        LOG("Executing test '%s'...", tc->name);
        int rc = lua_pcall(L, 0, 0, erridx);
        LOG("Finished executing test '%s', status: %d", tc->name, rc);

        char *file = NULL;
        int line = 0;
        char *trace = NULL;

        if (rc != LUA_OK) {
            trace = strdup(lua_tostring(L, -1)); /* traceback string */
            LOG("Test '%s' traceback: %s", tc->name, trace);

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

        if (rc == LUA_OK) {
            if (test_marked_failed) {
                taf_state_test_failed(state, NULL, 0, NULL);
            } else {
                taf_state_test_passed(state);
                passed++;
            }
        } else {
            taf_state_test_failed(state, file ? file : "unknown", line,
                                  trace ? trace : "unknown");
            free(file);
            free(trace);
        }

        run_deferred(L, state, rc == LUA_OK ? "passed" : "failed");

        taf_hooks_run(L, TAF_HOOK_FN_TEST_FINISHED);
    }

    taf_hooks_run(L, TAF_HOOK_FN_TEST_RUN_FINISHED);
    taf_state_test_run_finished(state);

    return passed == amount ? EXIT_SUCCESS : EXIT_FAILURE;
}

static char *get_lib_dir() {
    LOG("Getting TAF library directory location...");

    // Check argument first
    cmd_test_options *opts = cmd_parser_get_test_options();
    if (opts->custom_taf_lib_path) {
        LOG("--taf-lib-path specified: %s", opts->custom_taf_lib_path);
        return strdup(opts->custom_taf_lib_path);
    }

    // Check 'TAF_LIB_PATH' env variable
    char *env_path = getenv("TAF_LIB_PATH");
    if (env_path && *env_path) {
        LOG("TAF_LIB_PATH specified: %s", env_path);
        fprintf(stdout, "TAF_LIB_PATH is set.\n");
        fprintf(stdout, "Loading taf lua library at path '%s'\n", env_path);

        return env_path;
    }

    // Check TAF_DIR_PATH if TAF was installed with package manager
    if (strlen(TAF_DIR_PATH) != 0) {
        LOG("LUA_INSTALL_DIR specified: %s", TAF_DIR_PATH);
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s/lib", TAF_DIR_PATH);
        return strdup(path);
    }

    LOG("LUA_INSTALL_DIR not specified, trying '~/.taf'...");
    const char *home_path = getenv("HOME");
    if (!home_path || !*home_path) {
        fprintf(stderr, "Unable to load taf lua library: 'HOME' "
                        "environment variable is not set.\n"
                        "Use --libpath path_to_taf_lib_dir or set "
                        "'TAF_LIB_PATH' environment variable.\n");
        LOG("Unable to find TAF library directory: HOME is not set.");
        internal_logging_deinit();
        exit(EXIT_FAILURE);
    }
    LOG("HOME path: %s", home_path);
    char default_path[PATH_MAX];
    snprintf(default_path, PATH_MAX, "%s/.taf/lib", home_path);
    LOG("TAF library path: %s", default_path);

    return strdup(default_path);
}

static void inject_modules_dir(lua_State *L) {
    LOG("Injecting TAF library directory...");

    module_path = get_lib_dir();

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path"); /* pkg.path string */
    const char *old_path = lua_tostring(L, -1);
    if (directory_exists(test_common_dir_path)) {
        lua_pushfstring(L,
                        "%s;%s/?.lua;%s/?/init.lua;%s/?.lua;%s/?/init.lua;%s/"
                        "?.lua;%s/?/init.lua;%s/?.lua;%s/?/init.lua",
                        old_path, module_path, module_path, lib_dir_path,
                        lib_dir_path, test_dir_path, test_dir_path,
                        test_common_dir_path, test_common_dir_path);
    } else {
        lua_pushfstring(L,
                        "%s;%s/?.lua;%s/?/init.lua;%s/?.lua;%s/?/init.lua;%s/"
                        "?.lua;%s/?/init.lua",
                        old_path, module_path, module_path, lib_dir_path,
                        lib_dir_path, test_dir_path, test_dir_path);
    }
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
    register_clua_module(L, "taf-hooks", l_module_hooks_register_module);

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

static int load_lua_dir(const char *dir_path, lua_State *L) {
    if (!directory_exists(dir_path)) {
        LOG("Directory %s doesn't exist.", dir_path);
        return -1;
    }

    str_array_t lua_files = list_lua_recursive(dir_path);
    if (lua_files.count != 0) {
        LOG("Found lua files in '%s', loading...", dir_path);

        if (load_lua_files(L, &lua_files)) {
            free_str_array(&lua_files);
            return -2;
        }

        free_str_array(&lua_files);
        return 0;
    } else {
        LOG("No lua files found in '%s'.", dir_path);
        free_str_array(&lua_files);
        return -1;
    }
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

    const unsigned long min_version_required =
        TAF_VERSION_NUM(proj->min_taf_ver_major, proj->min_taf_ver_minor,
                        proj->min_taf_ver_patch);
    if (min_version_required > TAF_VERSION_NUM_CURRENT) {
        printf("\x1b[33mWARNING: This project was created with a newer TAF "
               "version '%s' "
               "(current TAF version '%s'), some features might not work as "
               "expected.\n\x1b[0m",
               proj->min_taf_ver_str, TAF_VERSION);
    }

    if (!opts->target && proj->multitarget) {
        fprintf(stderr, "Project is multitarget, but no target specified.\n");
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
    if (opts->target) {
        bool found = false;
        for (size_t i = 0; i < proj->targets_amount; i++) {
            if (!strcmp(opts->target, proj->targets[i])) {
                found = true;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Target '%s' was not found.\n", opts->target);
            LOG("Target %s was not found.", opts->target);
            internal_logging_deinit();
            return EXIT_FAILURE;
        }
    }

    LOG("Creating Lua state...");
    lua_State *L = luaL_newstate();
    LOG("Opening Lua libs...");
    luaL_openlibs(L);

    asprintf(&lib_dir_path, "%s/lib", proj->project_path);
    if (proj->multitarget) {
        asprintf(&test_common_dir_path, "%s/tests/common", proj->project_path);
        asprintf(&test_dir_path, "%s/tests/%s", proj->project_path,
                 opts->target);
    } else {
        asprintf(&test_dir_path, "%s/tests", proj->project_path);
    }

    register_test_api(L);

    int exitcode = EXIT_FAILURE;

    taf_state_t *state = NULL;

    LOG("Project lib directory path: %s", lib_dir_path);
    if (load_lua_dir(lib_dir_path, L) == -2) {
        goto deinit;
    }

    if (proj->multitarget) {
        if (load_lua_dir(test_common_dir_path, L) == -2) {
            goto deinit;
        }
    }
    if (load_lua_dir(test_dir_path, L) == -2) {
        goto deinit;
    }

    if (taf_parse_vars()) {
        goto deinit;
    }

    if (taf_parse_secrets()) {
        goto deinit;
    }

    size_t amount = da_size(test_case_get_all());

    if (amount == 0) {
        LOG("No tests found.");
        fprintf(stderr, "No tests to execute.\n");
        goto deinit;
    }

    state = taf_state_new();

    l_module_taf_init(state);

    if (!opts->no_logs) {
        taf_log_init(state);
    }

    taf_hooks_init(state);
    asprintf(&hooks_dir_path, "%s/hooks", proj->project_path);
    if (load_lua_dir(hooks_dir_path, L) == -2) {
        goto deinit;
    }

    if (!opts->headless && taf_tui_init(state)) {
        goto deinit;
    }

    if (opts->headless) {
        taf_headless_init(state);
    }

    if (!opts->headless) {
        LOG("Enabling line hook...");
        lua_sethook(L, line_hook, LUA_MASKLINE, 0);
    }

    exitcode = run_all_tests(L, state);

    if (!opts->headless) {
        taf_tui_deinit();
    }

deinit:

    LOG("Tidying up...");

    test_case_free_all(L);
    taf_hooks_deinit(L);
    lua_close(L);
    project_parser_free();
    internal_logging_deinit();
    taf_free_vars();
    taf_free_secrets();
    taf_state_free(state);
    free(hooks_dir_path);
    free(module_path);
    free(test_common_dir_path);
    free(test_dir_path);
    free(lib_dir_path);
    cmd_parser_free_test_options();

    return exitcode;
}
