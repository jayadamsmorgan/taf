#include "taf_vars.h"

#include "cmd_parser.h"

#include "util/kv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static da_t *vars = NULL;

void taf_register_vars(da_t *new) {
    if (!new)
        return;

    if (!vars)
        vars = da_init(1, sizeof(taf_var_entry_t));

    size_t new_size = da_size(new);
    for (size_t i = 0; i < new_size; ++i) {
        taf_var_entry_t *e = da_get(new, i);
        da_append(vars, e);
    }

    da_free(new);
}

da_t *taf_get_vars() { return vars; }

int taf_parse_vars() {

    int res = 0;

    cmd_test_options *opts = cmd_parser_get_test_options();
    size_t opts_vars_count = da_size(opts->vars);
    for (size_t i = 0; i < opts_vars_count; ++i) {
        for (size_t j = 0; j < opts_vars_count; ++j) {
            if (i == j)
                continue;
            kv_pair_t *l = da_get(opts->vars, i);
            kv_pair_t *r = da_get(opts->vars, j);
            if (strcmp(l->key, r->key) == 0) {
                fprintf(stderr,
                        "Error: value for variable '%s' was specified more "
                        "than once.\n",
                        l->key);
                return -1;
            }
        }
    }

    size_t registered_count = da_size(vars);
    for (size_t i = 0; i < registered_count; ++i) {
        taf_var_entry_t *e = da_get(vars, i);
        kv_pair_t *found = NULL;
        for (size_t j = 0; j < opts_vars_count; ++j) {
            kv_pair_t *p = da_get(opts->vars, j);
            if (strcmp(e->name, p->key) == 0) {
                found = p;
                break;
            }
        }
        if (e->is_scalar) {
            if (found) {
                printf("Warning: scalar variable '%s' redefined.\n", e->name);
                e->final_value = strdup(found->value);
                continue;
            }
            e->final_value = strdup(e->scalar);
            continue;
        }

        if (!found && e->def_value) {
            e->final_value = strdup(e->def_value);
            continue;
        }

        if (!found) {
            fprintf(stderr,
                    "Error: Value for variable '%s' is not specified. Please "
                    "specify it with `--vars %s=value` or add default value in "
                    "declaration.\n",
                    e->name, e->name);
            res = -1;
            continue;
        }

        size_t values_count = da_size(e->values);

        if (values_count == 0) {
            e->final_value = strdup(found->value);
            continue;
        }

        for (size_t j = 0; j < values_count; ++j) {
            char **value = da_get(e->values, j);
            if (strcmp(*value, found->value) == 0) {
                // found
                e->final_value = strdup(found->value);
                break;
            }
        }
        if (e->final_value) {
            continue;
        }

        fprintf(stderr,
                "Error: Value for variable '%s' is not an allowed value.\n",
                e->name);
        res = -1;
    }

    return res;
}

void taf_free_vars() {
    if (!vars)
        return;
    size_t vars_count = da_size(vars);
    for (size_t i = 0; i < vars_count; ++i) {
        taf_var_entry_t *e = da_get(vars, i);
        free(e->scalar);
        free(e->def_value);
        free(e->name);
        free(e->final_value);
        size_t values_count = da_size(e->values);
        for (size_t j = 0; j < values_count; ++j) {
            char **value = da_get(e->values, j);
            free(*value);
        }
        da_free(e->values);
    }
    da_free(vars);
    vars = NULL;
}
