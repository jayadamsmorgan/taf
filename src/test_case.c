#include "test_case.h"

#include "internal_logging.h"

#include <stdlib.h>
#include <string.h>

static test_case_t *tests = NULL;
static size_t tests_len = 0;
static size_t tests_cap = 0;

void test_case_enqueue(test_case_t *tc) {
    if (!tc) {
        LOG("Test case is NULL");
        return;
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
    memcpy(&tests[tests_len], tc, sizeof(test_case_t));
    tests_len++;
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
        LOG("Unrefing test '%s'", tests[i].name);
        luaL_unref(L, LUA_REGISTRYINDEX, tests[i].ref);
    }
    free(tests);
    tests_len = 0;
    LOG("Freeing tests OK.");
}
