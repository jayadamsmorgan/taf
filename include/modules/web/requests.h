#ifndef WEB_REQUESTS_H
#define WEB_REQUESTS_H

#include <curl/curl.h>
#include <json.h>

struct json_object *requests_http_post_json(const char *url, const char *body,
                                            char errbuf[CURL_ERROR_SIZE]);

struct json_object *requests_http_delete_json(const char *url,
                                              char errbuf[CURL_ERROR_SIZE]);

#endif // WEB_REQUESTS_H
