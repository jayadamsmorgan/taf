#ifndef MODULE_HTTP_H
#define MODULE_HTTP_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <curl/curl.h>

typedef struct {
    CURL *h;
    struct curl_slist *headers; // current header list (nullable)
    int write_ref;              // Lua registry ref for write callback
    int read_ref;               // idem for read callback
    lua_State *mainL;           // the "main" Lua state
} l_module_http_t;

/******************* API START ***********************/

// http:new() -> handle
int l_module_http_new(lua_State *L);

// handle:setopt(self:handle,opt:integer,value:integer|bool|function|string|[string])
// -> handle
int l_module_http_setopt(lua_State *L);

// handle:perform(self:handle)
int l_module_http_perform(lua_State *L);

/******************* API END *************************/

// Register "taf-http" module
int l_module_http_register_module(lua_State *L);

#endif // MODULE_HTTP_H
