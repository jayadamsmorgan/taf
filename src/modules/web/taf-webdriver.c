#include "modules/web/taf-webdriver.h"

#include "modules/web/webdriver.h"

#include "util/lua.h"

#include <string.h>

static wd_driver_backend str_to_wd_driver_backend(const char *str) {
    if (!strcmp(str, "chromedriver"))
        return WD_DRV_CHROMEDRIVER;
    if (!strcmp(str, "geckodriver"))
        return WD_DRV_GECKODRIVER;
    if (!strcmp(str, "safaridriver"))
        return WD_DRV_SAFARIDRIVER;
    if (!strcmp(str, "msedgedriver"))
        return WD_DRV_MSEDGEDRIVER;
    if (!strcmp(str, "iedriver"))
        return WD_DRV_IEDRIVER;
    if (!strcmp(str, "operadriver"))
        return WD_DRV_OPERADRIVER;
    if (!strcmp(str, "webkitdriver"))
        return WD_DRV_WEBKITDRIVER;
    if (!strcmp(str, "wpedriver"))
        return WD_DRV_WPEDRIVER;
    return -1;
}

int l_module_web_session_start(lua_State *L) {
    int s = selfshift(L);

    int port = luaL_checkinteger(L, s);
    const char *driver = luaL_checkstring(L, s + 1);
    wd_driver_backend backend = str_to_wd_driver_backend(driver);
    if (backend < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Specified driver was not found.");
        return 2;
    }

    luaL_checktype(L, s + 2, LUA_TTABLE);
    size_t len = lua_rawlen(L, s + 2);
    char **args = malloc(sizeof(*args) * (len + 1));
    if (!args) {
        lua_pushnil(L);
        lua_pushstring(L, "malloc: out of memory");
        return 2;
    }

    for (size_t i = 0; i < len; i++) {
        lua_geti(L, s + 2, i + 1);
        size_t str_len;
        const char *arg = luaL_checklstring(L, -1, &str_len);
        args[i] = malloc(sizeof(char) * (str_len + 1));
        if (!args[i]) {
            lua_pushnil(L);
            lua_pushstring(L, "malloc: out of memory");
            return 2;
        }
        memcpy(args[i], arg, str_len + 1);
        lua_pop(L, 1);
    }

    args[len] = malloc(sizeof(char) * 30);
    if (!args[len]) {
        lua_pushnil(L);
        lua_pushstring(L, "malloc: out of memory");
        return 2;
    }
    snprintf(args[len], 30, "--port=%d", port);

    char errbuf[WD_ERRORSIZE];
    wd_pid_t driver_pid = wd_spawn_driver(backend, len + 1, args, errbuf);

    for (size_t i = 0; i < len + 1; i++) {
        free(args[i]);
    }
    free(args);

    if (driver_pid < 0) {
        lua_pushnil(L);
        lua_pushstring(L, errbuf);
        return 2;
    }

    wd_session_t *session = lua_newuserdata(L, sizeof *session);
    session->driver_pid = driver_pid;
    wd_status stat = wd_session_start(port, backend, errbuf, session);

    if (stat != WD_OK) {
        lua_pushnil(L);
        lua_pushstring(L, errbuf);
        return 2;
    }

    luaL_getmetatable(L, "taf-webdriver");
    lua_setmetatable(L, -2);
    lua_pushnil(L);
    return 2;
}

static wd_method wd_method_from_str(const char *str) {
    if (!strcmp(str, "post"))
        return WD_METHOD_POST;
    if (!strcmp(str, "put"))
        return WD_METHOD_PUT;
    if (!strcmp(str, "delete"))
        return WD_METHOD_DELETE;
    if (!strcmp(str, "get"))
        return WD_METHOD_GET;
    return -1;
}

int l_module_web_session_cmd(lua_State *L) {

    int s = selfshift(L);

    wd_session_t *session = luaL_checkudata(L, s, "taf-webdriver");

    const char *method_str = luaL_checkstring(L, s + 1);
    wd_method method = wd_method_from_str(method_str);
    if (method < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "unknown method");
        return 2;
    }

    const char *endpoint = luaL_checkstring(L, s + 2);

    luaL_checktype(L, s + 3, LUA_TTABLE);
    json_object *payload = lua_to_json(L, s + 3);

    json_object *out;
    wd_session_cmd(session, method, endpoint, payload, &out);

    json_to_lua(L, out);

    json_object_put(out);

    lua_pushnil(L);

    return 2;
}

int l_module_web_session_end(lua_State *L) {
    int s = selfshift(L);

    wd_session_t *session = luaL_checkudata(L, s, "taf-webdriver");

    wd_status stat = wd_session_end(session);
    if (stat != WD_OK) {
        const char *err = wd_status_to_str(stat);
        lua_pushstring(L, err);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

/*----------- registration ------------------------------------------*/
static const luaL_Reg module_fns[] = {
    {"session_cmd", l_module_web_session_cmd},
    {"session_end", l_module_web_session_end},
    {"session_start", l_module_web_session_start},
    {NULL, NULL},
};

static int l_gc(lua_State *L) { return l_module_web_session_end(L); }

static const luaL_Reg port_mt[] = {{"__gc", l_gc}, {NULL, NULL}};

int l_module_web_register_module(lua_State *L) {
    /* metatable for port userdata */
    luaL_newmetatable(L, "taf-webdriver");
    luaL_setfuncs(L, port_mt, 0);
    lua_pop(L, 1);

    /* module table */
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    return 1;
}
