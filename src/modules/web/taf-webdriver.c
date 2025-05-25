#include "modules/web/taf-webdriver.h"

#include "modules/web/webdriver.h"

#include "util/lua.h"

#include <string.h>

static inline int selfshift(lua_State *L) { /* 1 = dot‑call, 2 = colon‑call */
    return lua_istable(L, 1) ? 2 : 1;
}

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

    const char *extra_args = luaL_optstring(L, s + 2, NULL);
    char argbuff[1024];
    // TODO: Fix args (args should be an array of strings instead)
    if (extra_args && *extra_args) {
        snprintf(argbuff, 1024, "--port=%d %s", port, extra_args);
    } else {
        snprintf(argbuff, 1024, "--port=%d", port);
    }
    char errbuf[WD_ERRORSIZE];
    wd_pid_t driver_pid = wd_spawn_driver(backend, argbuff, errbuf);
    if (driver_pid < 0) {
        lua_pushnil(L);
        lua_pushstring(L, errbuf);
        return 2;
    }

    wd_session_t *session = lua_newuserdata(L, sizeof *session);
    session->driver_pid = driver_pid;
    wd_status stat = wd_session_start(port, backend, session);
    if (stat != WD_OK) {
        const char *err = wd_status_to_str(stat);
        lua_pushnil(L);
        lua_pushstring(L, err);
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

static const luaL_Reg module_fns[] = {
    {"session_start", l_module_web_session_start},
    {"session_end", l_module_web_session_end},
    {"session_cmd", l_module_web_session_cmd},
    {NULL, NULL},
};

static int l_gc(lua_State *) { return 0; }

/*----------- registration ------------------------------------------*/
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
