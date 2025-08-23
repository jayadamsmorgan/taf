#include "test_case.h"

#include "cmd_parser.h"
#include "internal_logging.h"

#include <stdlib.h>
#include <string.h>

static test_case_t *tests = NULL;
static size_t tests_len = 0;
static size_t tests_cap = 0;

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
    for (size_t i = 0; i < tests_len; i++) {
        if (!strcmp(tc->name, tests[i].name)) {
            LOG("Overwriting test '%s'...", tc->name);
            test_case_free(L, &tests[i]);

            tests[i] = *tc;
            free(tc);
            return 0;
        }
    }
    LOG("Adding test '%s' to the queue", tc->name);
    if (tests_len == tests_cap) { /* grow 2× */
        LOG("Reallocating: cap: %zu, len: %zu", tests_cap, tests_len);
        tests_cap = tests_cap ? tests_cap * 2 : 8;
        tests = realloc(tests, tests_cap * sizeof(test_case_t));
        if (!tests) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
    }
    tests[tests_len] = *tc;
    tests_len++;
    free(tc);

    return 0;
}

test_case_t *test_case_get_all(size_t *amount) {
    if (amount) {
        *amount = tests_len;
    }
    return tests;
}

void test_case_free_all(lua_State *L) {
    LOG("Freeing test cases...");
    if (!tests) {
        LOG("Tests are null.");
        return;
    }
    for (size_t i = 0; i < tests_len; i++) {
        LOG("Freeing test '%s'", tests[i].name);
        test_case_free(L, &tests[i]);
    }
    free(tests);
    tests = NULL;
    tests_len = 0;
    tests_cap = 0;
    LOG("Freeing tests OK.");
}
