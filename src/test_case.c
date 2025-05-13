#include "test_case.h"

#include <stdlib.h>
#include <string.h>

static test_case_t *tests = NULL;
static size_t tests_len = 0;
static size_t tests_cap = 0;

void test_case_enqueue(test_case_t *tc) {
    if (!tc) {
        return;
    }
    if (tests_len == tests_cap) { /* grow 2× */
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
    if (!tests) {
        return;
    }
    for (size_t i = 0; i < tests_len; i++) {
        luaL_unref(L, LUA_REGISTRYINDEX, tests[i].ref);
    }
    free(tests);
    tests_len = 0;
}
