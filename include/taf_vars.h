#ifndef TAF_VARS_H
#define TAF_VARS_H

#include "util/da.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *name;

    bool is_scalar; /* true if original value was a string */
    char *scalar;   /* set when is_scalar == true */

    da_t *values;    /* optional, duplicated strings */
    char *def_value; /* optional default string */

    char *final_value;
} taf_var_entry_t;

void taf_register_vars(da_t *vars);

int taf_parse_vars();

da_t *taf_get_vars();

void taf_free_vars();

#endif // TAF_VARS_H
