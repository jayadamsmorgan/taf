#ifndef TESTS_H
#define TESTS_H

#include <unistd.h>

#include <lauxlib.h>

typedef struct {
    const char *name; /* test name             */
    int ref;          /* reference to Lua fn   */
} test_case_t;

void test_case_enqueue(test_case_t *tc);

test_case_t *test_case_get_all(size_t *amount);

void test_case_free_all(lua_State *L);

#endif // TESTS_H
