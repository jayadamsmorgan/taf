#include "modules/web/requests.h"

#include <stdlib.h>
#include <string.h>

struct mem {
    char *data;
    size_t len, cap;
};

static size_t grow_cb(char *ptr, size_t size, size_t nmemb, void *ud) {
    struct mem *m = ud;
    size_t add = size * nmemb;
    if (m->len + add + 1 >= m->cap) {
        m->cap = (m->len + add + 1) * 2;
        m->data = realloc(m->data, m->cap);
    }
    memcpy(m->data + m->len, ptr, add);
    m->len += add;
    return add;
}

struct json_object *requests_http_post_json(const char *url, const char *body,
                                            char errbuf[CURL_ERROR_SIZE]) {
    CURL *c = curl_easy_init();
    if (!c) {
        strcpy(errbuf, "curl_init_failed");
        return NULL;
    }

    struct mem buf = {.data = malloc(1024), .len = 0, .cap = 1024};

    struct curl_slist *hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");

    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, grow_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(c, CURLOPT_ERRORBUFFER, errbuf);

    struct json_object *out = NULL;
    if (curl_easy_perform(c) == CURLE_OK) {
        buf.data[buf.len] = '\0';
        out = json_tokener_parse(buf.data);
        if (!out)
            strcpy(errbuf, "JSON parse fail");
    }

    curl_easy_cleanup(c);
    curl_slist_free_all(hdrs);
    free(buf.data);
    return out;
}

struct json_object *requests_http_delete_json(const char *url,
                                              char errbuf[CURL_ERROR_SIZE]) {
    CURL *c = curl_easy_init();
    if (!c) {
        strcpy(errbuf, "curl_init_failed");
        return NULL;
    }

    struct mem buf = {.data = malloc(1024), .len = 0, .cap = 1024};

    struct curl_slist *hdrs = NULL;

    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, grow_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(c, CURLOPT_ERRORBUFFER, errbuf);

    struct json_object *out = NULL;
    if (curl_easy_perform(c) == CURLE_OK) {
        buf.data[buf.len] = '\0';
        out = json_tokener_parse(buf.data);
        if (!out)
            strcpy(errbuf, "JSON parse fail");
    }

    curl_easy_cleanup(c);
    curl_slist_free_all(hdrs);
    free(buf.data);
    return out; // NULL on failure
}

struct json_object *requests_http_put_json(const char *url, const char *body,
                                           char errbuf[CURL_ERROR_SIZE]) {
    CURL *c = curl_easy_init();
    if (!c) {
        strcpy(errbuf, "curl_init_failed");
        return NULL;
    }

    struct mem buf = {.data = malloc(1024), .len = 0, .cap = 1024};

    struct curl_slist *hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");

    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, grow_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(c, CURLOPT_ERRORBUFFER, errbuf);

    struct json_object *out = NULL;
    if (curl_easy_perform(c) == CURLE_OK) {
        buf.data[buf.len] = '\0';
        out = json_tokener_parse(buf.data);
        if (!out)
            strcpy(errbuf, "JSON parse fail");
    }

    curl_easy_cleanup(c);
    curl_slist_free_all(hdrs);
    free(buf.data);
    return out;
}

struct json_object *requests_http_get_json(const char *url,
                                           char errbuf[CURL_ERROR_SIZE]) {
    CURL *c = curl_easy_init();
    if (!c) {
        strcpy(errbuf, "curl_init_failed");
        return NULL;
    }

    struct mem buf = {.data = malloc(1024), .len = 0, .cap = 1024};

    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, grow_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(c, CURLOPT_ERRORBUFFER, errbuf);

    struct json_object *out = NULL;
    if (curl_easy_perform(c) == CURLE_OK) {
        buf.data[buf.len] = '\0';
        out = json_tokener_parse(buf.data);
        if (!out)
            strcpy(errbuf, "JSON parse fail");
    }

    curl_easy_cleanup(c);
    free(buf.data);
    return out;
}
