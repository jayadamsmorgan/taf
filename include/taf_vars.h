#ifndef TAF_VARS_H
#define TAF_VARS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *name;     /* key */
    bool is_scalar; /* true if original value was a string */
    char *scalar;   /* set when is_scalar == true */

    char **values; /* optional, duplicated strings */
    size_t values_amount;
    char *def_value; /* optional default string */

    char *final_value;
} taf_var_entry_t;

typedef struct {
    taf_var_entry_t *entries;
    size_t count;
} taf_var_map_t;

void taf_register_vars(taf_var_map_t *map);

int taf_parse_vars();

taf_var_map_t *taf_get_vars();

#endif // TAF_VARS_H
