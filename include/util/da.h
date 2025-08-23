#ifndef UTIL_DA_H
#define UTIL_DA_H

#include <stdbool.h>
#include <stddef.h>

typedef struct da_t da_t;

da_t *da_init(size_t init_capacity, size_t elem_size); // elem_size > 0
void da_free(da_t *da);

bool da_append(da_t *da, const void *elem);        // returns false on OOM
void *da_get(da_t *da, size_t index);              // NULL if OOB
const void *da_cget(const da_t *da, size_t index); // const view, NULL if OOB

size_t da_size(const da_t *da);
size_t da_capacity(const da_t *da);

bool da_reserve(da_t *da,
                size_t min_capacity); // grow to at least min_capacity
void da_clear(da_t *da);              // keep capacity, reset size to 0

#endif /* UTIL_DA_H */
