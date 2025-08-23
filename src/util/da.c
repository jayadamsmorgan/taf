#include "util/da.h"

#include <stdlib.h>
#include <string.h>

struct da_t {
    void *data;
    size_t elem_size;
    size_t size;
    size_t capacity;
};

static bool da_grow_for_one(da_t *da) {
    if (da->size < da->capacity)
        return true;

    size_t new_cap = da->capacity ? da->capacity * 2 : 1;

    if (new_cap < da->capacity)
        return false; // overflow
    return da_reserve(da, new_cap);
}

da_t *da_init(size_t init_capacity, size_t elem_size) {
    if (elem_size == 0)
        return NULL;

    da_t *da = (da_t *)calloc(1, sizeof *da);
    if (!da)
        return NULL;

    da->elem_size = elem_size;
    da->capacity = init_capacity;
    da->size = 0;

    if (init_capacity) {
        da->data = malloc(init_capacity * elem_size);
        if (!da->data) {
            free(da);
            return NULL;
        }
    }
    return da;
}

void da_free(da_t *da) {
    if (!da)
        return;
    free(da->data);
    free(da);
}

bool da_reserve(da_t *da, size_t min_capacity) {
    if (!da)
        return false;
    if (min_capacity <= da->capacity)
        return true;

    if (da->elem_size != 0 && min_capacity > SIZE_MAX / da->elem_size) {
        return false;
    }

    void *newp = realloc(da->data, min_capacity * da->elem_size);
    if (!newp)
        return false;

    da->data = newp;
    da->capacity = min_capacity;
    return true;
}

bool da_append(da_t *da, const void *elem) {
    if (!da || !elem)
        return false;
    if (!da_grow_for_one(da))
        return false;

    memcpy((char *)da->data + da->size * da->elem_size, elem, da->elem_size);
    da->size++;
    return true;
}

void *da_get(da_t *da, size_t index) {
    if (!da || index >= da->size)
        return NULL;
    return (char *)da->data + index * da->elem_size;
}

const void *da_cget(const da_t *da, size_t index) {
    if (!da || index >= da->size)
        return NULL;
    return (const char *)da->data + index * da->elem_size;
}

size_t da_size(const da_t *da) {
    //
    return da ? da->size : 0;
}

size_t da_capacity(const da_t *da) {
    //
    return da ? da->capacity : 0;
}

void da_clear(da_t *da) {
    if (!da)
        return;
    da->size = 0;
}
