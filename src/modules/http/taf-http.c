#include "modules/http/taf-http.h"

#include "internal_logging.h"

#include "util/lua.h"

#include <string.h>

static void ud_clear_slist(l_module_http_t *handle) {
    LOG("Clearing slist...");
    if (handle->headers) {
        curl_slist_free_all(handle->headers);
        handle->headers = NULL;
    }
    LOG("Successfully cleared slist.");
}

static struct curl_slist *lua_to_slist(lua_State *L, int idx) {
    LOG("Converting Lua string array into curl_slist...");
    struct curl_slist *head = NULL;
    size_t n = lua_rawlen(L, idx);
    for (size_t i = 1; i < n; i++) {
        lua_geti(L, idx, i);
        const char *line = luaL_checkstring(L, -1);
        head = curl_slist_append(head, line);
        lua_pop(L, 1);
    }

    LOG("Successfully converted.");
    return head;
}

int l_module_http_new(lua_State *L) {
    LOG("Invoked taf-http new...");
    l_module_http_t *ud = lua_newuserdata(L, sizeof *ud);
    memset(ud, 0, sizeof *ud);
    ud->h = curl_easy_init();
    ud->mainL = L;
    if (!ud) {
        LOG("curl_easy_init() failed");
        return luaL_error(L, "curl_easy_init() failed");
    }

    luaL_getmetatable(L, "taf-http");
    lua_setmetatable(L, -2);
    LOG("Successfully finished taf-http new.");
    return 1;
}

static size_t c_write_cb(char *ptr, size_t size, size_t nmemb, void *ud_) {
    LOG("CURL WRITE cb invoked...");
    size_t nbytes = size * nmemb;
    l_module_http_t *ud = (l_module_http_t *)ud_;

    if (ud->write_ref == LUA_NOREF) { // no Lua handler
        LOG("No registered write cb.");
        return nbytes; // tell curl “all consumed”
    }

    lua_State *L = ud->mainL; // the main state
    int top = lua_gettop(L);  // stack guard

    lua_rawgeti(L, LUA_REGISTRYINDEX, ud->write_ref); // push Lua fn
    lua_pushlstring(L, ptr, nbytes);                  // arg #1 data
    lua_pushinteger(L, (lua_Integer)nbytes);          // arg #2 size

    if (lua_pcall(L, 2, 1, 0) != LUA_OK) { // call fn(data,size)
        LOG("curl write-cb Lua error: %s\n", lua_tostring(L, -1));
        lua_settop(L, top);
        return 0; // returning 0 -> abort
    }

    // Lua result: how many bytes we really consumed
    size_t taken = (size_t)lua_tointeger(L, -1);
    LOG("Taken: %zu", taken);

    lua_settop(L, top); // restore stack

    LOG("CURL WRITE cb finished.");
    return taken;
}

static size_t c_read_cb(char *dest, size_t size, size_t nmemb, void *ud_) {
    LOG("CURL READ cb started...");
    size_t room = size * nmemb; // how many bytes curl wants
    LOG("Room: %zu", room);
    l_module_http_t *ud = (l_module_http_t *)ud_;

    if (ud->read_ref == LUA_NOREF) {
        LOG("No registered read cb.");
        return 0; // nothing to read
    }

    lua_State *L = ud->mainL;
    int top = lua_gettop(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, ud->read_ref); // push Lua fn
    lua_pushinteger(L, (lua_Integer)room);           // arg = max size
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        LOG("curl read-cb Lua error: %s\n", lua_tostring(L, -1));
        lua_settop(L, top);
        return CURL_READFUNC_ABORT; // abort upload
    }

    size_t len;
    const char *src = lua_tolstring(L, -1, &len);
    if (!src)
        len = 0; // nil/false -> EOF
    if (len > room)
        len = room; // clip

    LOG("Provided: %zu", len);

    memcpy(dest, src, len);
    lua_settop(L, top);

    LOG("CURL READ cb finished.");
    return len; // how many bytes we provided
}

int l_module_http_setopt(lua_State *L) {
    LOG("Invoked taf-http setopt...");
    l_module_http_t *ud = luaL_checkudata(L, 1, "taf-http");
    long option = luaL_checkinteger(L, 2);
    int vtype = lua_type(L, 3);
    LOG("Option: %lu, vtype: %d", option, vtype);
    CURLcode rc = CURLE_OK;

    switch (vtype) {

    case LUA_TNUMBER: // long / off_t
    case LUA_TBOOLEAN:
        rc = curl_easy_setopt(ud->h, option, (long)lua_tointeger(L, 3));
        break;

    case LUA_TSTRING: // const char*
        rc = curl_easy_setopt(ud->h, option, lua_tostring(L, 3));
        break;

    case LUA_TTABLE: // curl_slist
        if (option != CURLOPT_HTTPHEADER && option != CURLOPT_QUOTE) {
            LOG("table only allowed for HTTPHEADER/QUOTE");
            return luaL_error(L, "table only allowed for HTTPHEADER/QUOTE");
        }

        ud_clear_slist(ud); // free old list, if any
        ud->headers = lua_to_slist(L, 3);
        rc = curl_easy_setopt(ud->h, option, ud->headers);
        break;

    case LUA_TFUNCTION: { // callback

        // We need a small C trampoline because libcurl works with C
        // function pointers, not Lua closures.  One trampoline is enough
        // for both READ & WRITE – we distinguish with userdata stored in
        // curl via CURLOPT_*DATA

        int ref = luaL_ref(L, LUA_REGISTRYINDEX);

        if (option == CURLOPT_WRITEFUNCTION) {
            LOG("Registering write function...");
            if (ud->write_ref)
                luaL_unref(L, LUA_REGISTRYINDEX, ud->write_ref);
            ud->write_ref = ref;
            curl_easy_setopt(ud->h, CURLOPT_WRITEDATA, ud);
            rc = curl_easy_setopt(ud->h, CURLOPT_WRITEFUNCTION, c_write_cb);
            LOG("Successfully registered write function.");
        } else if (option == CURLOPT_READFUNCTION) {
            LOG("Registering read function...");
            if (ud->read_ref)
                luaL_unref(L, LUA_REGISTRYINDEX, ud->read_ref);
            ud->read_ref = ref;
            curl_easy_setopt(ud->h, CURLOPT_READDATA, ud);
            rc = curl_easy_setopt(ud->h, CURLOPT_READFUNCTION, c_read_cb);
            LOG("Successfully registered read function.");
        } else {
            LOG("function only allowed for READ/WRITE cb");
            return luaL_error(L, "function only allowed for READ/WRITE cb");
        }

        break;
    }

    default:
        LOG("unsupported Lua type for setopt()");
        return luaL_error(L, "unsupported Lua type for setopt()");
    }

    if (rc != CURLE_OK) {
        const char *err = curl_easy_strerror(rc);
        LOG("curl_easy_setopt: %s", err);
        return luaL_error(L, "curl_easy_setopt: %s", err);
    }

    lua_settop(L, 1); // method-chain

    LOG("Successfully finshed taf-http setopt.");
    return 1;
}

int l_module_http_perform(lua_State *L) {
    LOG("Invoked taf-http perform...");
    int s = selfshift(L);
    CURL **ud = luaL_checkudata(L, s, "taf-http");
    CURLcode rc = curl_easy_perform(*ud);
    if (rc != CURLE_OK) {
        const char *err = curl_easy_strerror(rc);
        LOG("curl_easy_perform: %s", err);
        return luaL_error(L, "curl_easy_perform: %s", err);
    }
    lua_pushboolean(L, 1);
    LOG("Successfully finished taf-http perform.");
    return 1;
}

int l_module_http_cleanup(lua_State *L) {
    LOG("Invoked taf-http cleanup...");
    int s = selfshift(L);
    CURL **ud = luaL_checkudata(L, s, "taf-http");
    if (*ud)
        curl_easy_cleanup(*ud);
    LOG("Successfully finished taf-http cleanup.");
    return 0;
}

static int l_module_http_gc(lua_State *L) {
    LOG("Invoked taf-http GC...");
    l_module_http_t *ud = luaL_checkudata(L, 1, "taf.curl.easy");
    if (!ud)
        return 0;

    ud_clear_slist(ud);
    if (ud->write_ref)
        luaL_unref(L, LUA_REGISTRYINDEX, ud->write_ref);
    if (ud->read_ref)
        luaL_unref(L, LUA_REGISTRYINDEX, ud->read_ref);

    if (ud->h)
        curl_easy_cleanup(ud->h);

    LOG("Successfully finished taf-http GC.");
    return 0;
}

/*----------- registration ------------------------------------------*/
static const luaL_Reg handle_fns[] = {
    {"setopt", l_module_http_setopt},   //
    {"perform", l_module_http_perform}, //
    {"cleanup", l_module_http_cleanup}, //
    {NULL, NULL},                       //
};

int l_module_http_register_module(lua_State *L) {
    LOG("Registering taf-http module...");

    luaL_newmetatable(L, "taf-http");

    LOG("Registering GC functions...");
    lua_pushcfunction(L, l_module_http_gc);
    lua_setfield(L, -2, "__gc");
    LOG("GC functions registered.");

    LOG("Registering handle functions..");
    lua_newtable(L);
    luaL_setfuncs(L, handle_fns, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    LOG("Handle functions registered.");

    LOG("Registering module functions...");
    lua_newtable(L);
    lua_pushcfunction(L, l_module_http_new);
    lua_setfield(L, -2, "new");
    LOG("Module functions registered.");

    LOG("Successfully registered registered.");

    LOG("Successfully registered taf-http module.");
    return 1;
}
