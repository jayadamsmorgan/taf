#include "taf_vars.h"

#include "cmd_parser.h"
#include "internal_logging.h"
#include "project_parser.h"
#include "util/files.h"
#include <string.h>

typedef struct {
    char *key;
    char *value;
} kv_pair_t;

typedef struct {
    kv_pair_t *items;
    size_t count;
} kv_list_t;

static taf_var_map_t *vars = NULL;

void taf_register_vars(taf_var_map_t *map) {
    //
    vars = map;
}

taf_var_map_t *taf_get_vars() { return vars; }

static char *rtrim(char *s) {
    if (!s)
        return s;
    size_t n = strlen(s);
    while (n && (unsigned char)s[n - 1] <= ' ')
        s[--n] = '\0';
    return s;
}
static char *ltrim(char *s) {
    if (!s)
        return s;
    size_t i = 0;
    while (s[i] && (unsigned char)s[i] <= ' ')
        i++;
    return s + i;
}

static char *trim(char *s) { return rtrim(ltrim(s)); }

static int sbuf_append(char **dst, size_t *len, size_t *cap, const char *src,
                       size_t add) {
    if (*len + add + 1 > *cap) {
        size_t ncap = (*cap ? *cap : 64);
        while (ncap < *len + add + 1)
            ncap *= 2;
        char *tmp = (char *)realloc(*dst, ncap);
        if (!tmp)
            return -1;
        *dst = tmp;
        *cap = ncap;
    }
    memcpy(*dst + *len, src, add);
    *len += add;
    (*dst)[*len] = '\0';
    return 0;
}

static int kv_add(kv_list_t *out, const char *key, const char *value) {
    kv_pair_t *ni =
        (kv_pair_t *)realloc(out->items, (out->count + 1) * sizeof(*ni));
    if (!ni)
        return -1;
    out->items = ni;
    out->items[out->count].key = strdup(key);
    out->items[out->count].value = strdup(value);
    if (!out->items[out->count].key || !out->items[out->count].value)
        return -1;
    out->count++;
    return 0;
}

void kv_list_free(kv_list_t *lst) {
    if (!lst)
        return;
    for (size_t i = 0; i < lst->count; i++) {
        free(lst->items[i].key);
        free(lst->items[i].value);
    }
    free(lst->items);
    lst->items = NULL;
    lst->count = 0;
}

// Helper to finalize a multiline value into the list and reset state.
static int finish_multiline(kv_list_t *out, char **ml_key, char **buf,
                            size_t *blen, size_t *bcap, int *in_multiline) {
    int rc = kv_add(out, *ml_key, (*buf) ? *buf : "");
    free(*ml_key);
    *ml_key = NULL;
    free(*buf);
    *buf = NULL;
    *blen = 0;
    *bcap = 0;
    *in_multiline = 0;
    return rc;
}

// Returns 0 on success; non-zero on error.
int parse_kv_file(const char *path, kv_list_t *out) {
    memset(out, 0, sizeof(*out));

    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;

    char *line = NULL;
    size_t n = 0;
    ssize_t m;

    int in_multiline = 0;
    char *ml_key = NULL;
    char *buf = NULL;
    size_t blen = 0, bcap = 0;

    while ((m = getline(&line, &n, f)) != -1) {
        // Strip CR/LF
        if (m > 0 && (line[m - 1] == '\n' || line[m - 1] == '\r')) {
            while (m > 0 && (line[m - 1] == '\n' || line[m - 1] == '\r'))
                line[--m] = '\0';
        }

        if (in_multiline) {
            const char *p = line;
            const char *clos = strstr(p, "\"\"\"");
            if (clos) {
                // Append content before closing delimiter
                if (clos > p) {
                    if (sbuf_append(&buf, &blen, &bcap, p,
                                    (size_t)(clos - p)) != 0)
                        goto oom;
                }
                if (finish_multiline(out, &ml_key, &buf, &blen, &bcap,
                                     &in_multiline) != 0)
                    goto oom;
                continue; // ignore trailing chars after closing """
            } else {
                if (*p) {
                    if (sbuf_append(&buf, &blen, &bcap, p, strlen(p)) != 0)
                        goto oom;
                }
                // Preserve original newline that getline ate
                if (sbuf_append(&buf, &blen, &bcap, "\n", 1) != 0)
                    goto oom;
                continue;
            }
        }

        // Not in multiline: ignore blank lines
        char *s = trim(line);
        if (*s == '\0')
            continue;

        // Expect key=value
        char *eq = strchr(s, '=');
        if (!eq)
            continue; // ignore malformed lines

        *eq = '\0';
        char *key = trim(s);
        char *val = trim(eq + 1);
        if (*key == '\0')
            continue;

        // Triple-quoted start?
        if (strncmp(val, "\"\"\"", 3) == 0) {
            val += 3;
            char *clos = strstr(val, "\"\"\"");
            if (clos) {
                *clos = '\0';
                if (kv_add(out, key, val) != 0)
                    goto oom;
            } else {
                ml_key = strdup(key);
                if (!ml_key)
                    goto oom;
                buf = NULL;
                blen = bcap = 0;
                if (*val) {
                    if (sbuf_append(&buf, &blen, &bcap, val, strlen(val)) != 0)
                        goto oom;
                    if (sbuf_append(&buf, &blen, &bcap, "\n", 1) != 0)
                        goto oom;
                }
                in_multiline = 1;
            }
        } else {
            if (kv_add(out, key, val) != 0)
                goto oom;
        }
    }

    if (in_multiline) {
        // EOF before closing """: finalize as-is (tolerant behavior).
        if (finish_multiline(out, &ml_key, &buf, &blen, &bcap, &in_multiline) !=
            0)
            goto oom;
    }

    fclose(f);
    free(line);
    return 0;

oom:
    fclose(f);
    free(line);
    free(ml_key);
    free(buf);
    kv_list_free(out);
    return -3;
}

int taf_parse_vars() {

    if (!vars) {
        return 0;
    }

    cmd_test_options *opts = cmd_parser_get_test_options();
    project_parsed_t *proj = get_parsed_project();
    char *secret_vars_path;
    asprintf(&secret_vars_path, "%s/.secrets", proj->project_path);
    kv_list_t list = {.count = 0};
    if (file_exists(secret_vars_path)) {
        parse_kv_file(secret_vars_path, &list);
    }

    for (size_t i = 0; i < opts->vars.count; i++) {
        bool contains = false;
        cmd_var_t *var = &opts->vars.args[i];
        for (size_t j = 0; j < list.count; j++) {
            kv_pair_t *pair = &list.items[j];
            if (strcmp(pair->key, var->name) == 0) {
                // Override secrets with cmd arg
                free(pair->value);
                pair->value = var->value;
                contains = true;
                break;
            }
        }
        if (contains)
            continue;
        // Add to list:
        kv_add(&list, var->name, var->value);
    }

    // Check for duplicates:
    for (size_t i = 0; i < list.count; i++) {
        for (size_t j = 1; j < list.count; j++) {
            if (i == j)
                continue;
            if (strcmp(list.items[i].key, list.items[j].key) == 0) {
                fprintf(stderr,
                        "Error: Value for variable '%s' was specified more "
                        "than once.\n",
                        list.items[i].key);
                return -1;
            }
        }
    }

    for (size_t i = 0; i < vars->count; i++) {
        taf_var_entry_t *e = &vars->entries[i];
        kv_pair_t *pair = NULL;
        LOG("Variable: %s", e->name);
        for (size_t j = 0; j < list.count; j++) {
            if (strcmp(list.items[j].key, e->name) == 0) {
                LOG("Found pair");
                pair = &list.items[j];
                break;
            }
        }
        if (e->is_scalar) {
            LOG("Scalar");
            if (pair) {
                printf("Warning: Scalar variable '%s' redefined.\n", e->name);
                e->final_value = strdup(pair->value);
                continue;
            }
            e->final_value = e->scalar;
            continue;
        }
        if (!pair) {
            if (!e->def_value) {
                LOG("No default value");
                fprintf(
                    stderr,
                    "Error: No value provided for variable '%s'. Either add "
                    "default value in the declaration, specify value with '-v "
                    "%s=value' or add it to the '.secrets' file.\n",
                    e->name, e->name);
                return -2;
            }
            LOG("Default value used");
            // Use default
            e->final_value = e->def_value;
            continue;
        }
        if (e->values_amount == 0) {
            LOG("Using any value");
            // Any value possible
            e->final_value = strdup(pair->value);
            continue;
        }
        // Check for allowed values
        bool found = false;
        for (size_t j = 0; j < e->values_amount; j++) {
            if (strcmp(pair->value, e->values[j]) == 0) {
                LOG("Found allowed");
                e->final_value = e->values[j];
                found = true;
                break;
            }
        }

        if (found)
            continue;

        LOG("Not allowed.");
        fprintf(stderr,
                "Error: Specified value for variable '%s' is not allowed. "
                "Allowed values (per variable declaration): [",
                e->name);
        for (size_t j = 0; j < e->values_amount; j++) {
            fprintf(stderr, " '%s'", e->values[j]);
            if (j < e->values_amount - 1) {
                fprintf(stderr, ",");
            }
        }
        fprintf(stderr, " ]\n");
        return -3;
    }

    kv_list_free(&list);

    return 0;
}
