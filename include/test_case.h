#ifndef TESTS_H
#define TESTS_H

#include "util/da.h"

#include <unistd.h>

#include <lauxlib.h>
#include <stdbool.h>

typedef struct {
    const char *name; /* test name           */
    const char *desc; /* test description    */
    da_t *tags;       /* test tags           */
    int ref;          /* reference to Lua fn */
} test_case_t;

int test_case_enqueue(lua_State *L, test_case_t *tc);

da_t *test_case_get_all();

void test_case_free_all(lua_State *L);

#endif // TESTS_H
