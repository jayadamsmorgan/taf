#ifndef TESTS_H
#define TESTS_H

#include "util/da.h"

#include <unistd.h>

#include <lauxlib.h>
#include <stdbool.h>

typedef struct {
    const char *name;
    const char *default_value;
    const char **values;
    size_t values_amount;
    bool any_value;
} test_var_t;

typedef struct {
    test_var_t *var;
    size_t var_amount;
} test_vars_t;

typedef struct {
    const char *name; /* test name           */
    const char *desc; /* test description    */
    da_t *tags;       /* test tags           */
    int ref;          /* reference to Lua fn */
} test_case_t;

int test_case_enqueue(lua_State *L, test_case_t *tc);

test_case_t *test_case_get_all(size_t *amount);

void test_case_free_all(lua_State *L);

#endif // TESTS_H
