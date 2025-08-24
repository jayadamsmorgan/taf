#include "test_case.h"

#include "cmd_parser.h"
#include "internal_logging.h"

#include <stdlib.h>
#include <string.h>

static da_t *tests = NULL;

static void test_case_free(lua_State *L, test_case_t *tc) {
    free((char *)tc->name);
    free((char *)tc->desc);
    size_t tags_amount = da_size(tc->tags);
    for (size_t i = 0; i < tags_amount; i++) {
        char **tag = da_get(tc->tags, i);
        free(*tag);
    }
    da_free(tc->tags);
    luaL_unref(L, LUA_REGISTRYINDEX, tc->ref);
}

int test_case_enqueue(lua_State *L, test_case_t *tc) {
    if (!tc) {
        LOG("Test case is NULL");
        return -1;
    }
    cmd_test_options *opts = cmd_parser_get_test_options();
    size_t opts_tags_amount = da_size(opts->tags);
    size_t test_tags_amount = da_size(tc->tags);
    if (opts_tags_amount > 0) {
        bool has_tag = false;
        for (size_t i = 0; i < opts_tags_amount; i++) {
            for (size_t j = 0; j < test_tags_amount; j++) {
                char **opts_tag = da_get(opts->tags, i);
                char **test_tag = da_get(tc->tags, j);
                if (strcmp(*opts_tag, *test_tag) == 0) {
                    has_tag = true;
                    break;
                }
            }
            if (has_tag)
                break;
        }
        if (!has_tag) {
            LOG("Skipping test '%s' registration, no tag found.", tc->name);
            test_case_free(L, tc);
            free(tc);
            return 1;
        }
    }
    if (!tests) {
        tests = da_init(1, sizeof(test_case_t));
    }
    size_t tests_count = da_size(tests);
    for (size_t i = 0; i < tests_count; i++) {
        test_case_t *registered = da_get(tests, i);
        if (!strcmp(tc->name, registered->name)) {
            LOG("Overwriting test '%s'...", tc->name);
            test_case_free(L, registered);
            registered->name = tc->name;
            registered->desc = tc->desc;
            registered->ref = tc->ref;
            registered->tags = tc->tags;
            free(tc);
            return 0;
        }
    }
    LOG("Adding test '%s' to the queue", tc->name);
    da_append(tests, tc);
    free(tc);

    return 0;
}

da_t *test_case_get_all() { return tests; }

void test_case_free_all(lua_State *L) {
    LOG("Freeing test cases...");
    if (!tests) {
        LOG("Tests are null.");
        return;
    }
    size_t tests_count = da_size(tests);
    for (size_t i = 0; i < tests_count; i++) {
        test_case_t *tc = da_get(tests, i);
        test_case_free(L, tc);
    }
    da_free(tests);
    tests = NULL;
    LOG("Freeing tests OK.");
}
