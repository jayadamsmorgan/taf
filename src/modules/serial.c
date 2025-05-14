#include "modules/serial.h"

#include <string.h>

/*----------- helpers ------------------------------------------------*/

static inline int push_result_nil(lua_State *L, enum sp_return r) {
    if (r != SP_OK) {
        lua_pushstring(L, sp_last_error_message());
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

static inline int selfshift(lua_State *L) { /* 1 = dot‑call, 2 = colon‑call */
    return lua_istable(L, 1) ? 2 : 1;
}

static inline l_module_serial_t *check_port(lua_State *L, int idx) {
    return luaL_checkudata(L, idx, "serial");
}

/* serial:get_port(path:string) -> port_ud,nil | nil,err */
int l_module_serial_get_port_by_name(lua_State *L) {
    int s = selfshift(L);
    const char *p = luaL_checkstring(L, s);

    l_module_serial_t *u = lua_newuserdata(L, sizeof *u);
    u->port = NULL;

    enum sp_return r = sp_get_port_by_name(p, &u->port);

    if (r != SP_OK) {
        lua_pushnil(L);
        lua_pushstring(L, sp_last_error_message());
        return 2;
    }

    luaL_getmetatable(L, "serial");
    lua_setmetatable(L, -2);
    lua_pushnil(L);
    return 2;
}

static inline enum sp_mode mode_from_str(const char *str) {
    if (!strcmp(str, "rw"))
        return SP_MODE_READ_WRITE;
    if (!strcmp(str, "r"))
        return SP_MODE_READ;
    if (!strcmp(str, "w"))
        return SP_MODE_WRITE;
    return -1;
}

/* serial:open(port:port_ud, mode:string) -> nil | err */
int l_module_serial_open(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    const char *mode_str = luaL_checkstring(L, s + 1);
    enum sp_mode mode = mode_from_str(mode_str);
    if (mode < 0) {
        lua_pushstring(L, "invalid mode, use 'r', 'w' or 'rw'");
        return 1;
    }

    return push_result_nil(L, sp_open(u->port, mode));
}

/* serial:set_baudrate(port:port_ud, baudrate:integer) -> nil | err */
int l_module_serial_set_baudrate(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    enum sp_return r = (sp_set_baudrate(u->port, luaL_checkinteger(L, s + 1)));

    return push_result_nil(L, r);
}

/* serial:set_bits(port:port_ud, bits:integer) -> nil | err */
int l_module_serial_set_bits(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    enum sp_return r = (sp_set_bits(u->port, luaL_checkinteger(L, s + 1)));

    return push_result_nil(L, r);
}

static inline enum sp_parity parity_from_str(const char *str) {
    if (!strcmp(str, "none"))
        return SP_PARITY_NONE;
    if (!strcmp(str, "odd"))
        return SP_PARITY_ODD;
    if (!strcmp(str, "even"))
        return SP_PARITY_EVEN;
    if (!strcmp(str, "mark"))
        return SP_PARITY_MARK;
    if (!strcmp(str, "space"))
        return SP_PARITY_SPACE;
    return SP_PARITY_INVALID;
}

/* serial:set_parity(port:port_ud, parity:string) -> nil | err */
int l_module_serial_set_parity(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    const char *str = luaL_checkstring(L, s + 1);
    enum sp_parity par = parity_from_str(str);
    if (par < 0) {
        lua_pushstring(
            L, "invalid parity, use 'none', 'odd', 'even', 'mark' or 'space'");
        return 1;
    }

    return push_result_nil(L, sp_set_parity(u->port, par));
}

/* serial:set_stopbits(port:port_ud, bits:integer) -> nil | err */
int l_module_serial_set_stopbits(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    enum sp_return r = (sp_set_stopbits(u->port, luaL_checkinteger(L, s + 1)));

    return push_result_nil(L, r);
}

/*----------- reading ------------------------------------------------*/
static inline int read_helper(lua_State *L, int blocking) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    int n = luaL_checkinteger(L, s + 1);
    int to_ms = luaL_optinteger(L, s + 2, 0);

    char *buf = lua_newuserdata(L, n);
    int got = blocking ? sp_blocking_read(u->port, buf, n, to_ms)
                       : sp_nonblocking_read(u->port, buf, n);

    if (got < 0) {
        lua_pushnil(L);
        lua_pushstring(L, sp_last_error_message());
        return 2;
    }

    lua_pushlstring(L, buf, got);
    lua_pushnil(L);
    return 2;
}

/* serial:read_blocking(port:port_ud, byte_amout:integer,
 * (opt)timeout:integer=0)
 * -> string, nil | nil, err */
int l_module_serial_read_blocking(lua_State *L) { return read_helper(L, 1); }

/* serial:read_nonblocking(port:port_ud, byte_amout:integer)
 * -> string, nil | nil, err */
int l_module_serial_read_nonblocking(lua_State *L) { return read_helper(L, 0); }

/*----------- writing ------------------------------------------------*/
int write_helper(lua_State *L, int blocking) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    size_t len;
    const char *buf = luaL_checklstring(L, s + 1, &len);
    int to_ms = luaL_optinteger(L, s + 2, 0);

    int wrote = blocking ? sp_blocking_write(u->port, buf, len, to_ms)
                         : sp_nonblocking_write(u->port, buf, len);

    if (wrote < 0) {
        lua_pushnil(L);
        lua_pushstring(L, sp_last_error_message());
        return 2;
    }

    lua_pushinteger(L, wrote);
    lua_pushnil(L);
    return 2;
}

/* serial:write_blocking(port:port_ud, string, (opt)timeout:integer=0)
 * -> integer, nil | nil, err */
int l_module_serial_write_blocking(lua_State *L) { return write_helper(L, 1); }

/* serial:write_nonblocking(port:port_ud, string) -> integer, nil | nil, err */
int l_module_serial_write_nonblocking(lua_State *L) {
    return write_helper(L, 0);
}

/*----------- status -------------------------------------------------*/
// int l_module_serial_get_input_waiting(lua_State *L) {
//     int s = selfshift(L);
//     l_module_serial_t *u = check_port(L, s);
//     enum sp_return r = sp_input_waiting(u->port);
//     if (r < SP_OK)
//         return push_sp_error(L);
//     lua_pushinteger(L, r);
//     return 1;
// }
//
// int l_module_serial_get_output_waiting(lua_State *L) {
//     int s = selfshift(L);
//     l_module_serial_t *u = check_port(L, s);
//     enum sp_return r = sp_output_waiting(u->port);
//     if (r < SP_OK)
//         return push_sp_error(L);
//     lua_pushinteger(L, r);
//     return 1;
// }

/*----------- flush / drain -----------------------------------------*/
// int l_module_serial_flush(lua_State *L) {
//     int s = selfshift(L);
//     l_module_serial_t *u = check_port(L, s);
//     const char *mode = luaL_optstring(L, s + 1, "both");
//     enum sp_buffer dir = !strcmp(mode, "input")    ? SP_BUF_INPUT
//                          : !strcmp(mode, "output") ? SP_BUF_OUTPUT
//                                                    : SP_BUF_BOTH;
//
//     enum sp_return r = sp_flush(u->port, dir);
//     if (r != SP_OK)
//         return push_sp_error(L);
//     lua_pushboolean(L, 1);
//     return 1;
// }
//
// int l_module_serial_drain(lua_State *L) {
//     int s = selfshift(L);
//     l_module_serial_t *u = check_port(L, s);
//     enum sp_return r = sp_drain(u->port);
//     if (r != SP_OK)
//         return push_sp_error(L);
//     lua_pushboolean(L, 1);
//     return 1;
// }

/*----------- close / GC --------------------------------------------*/
int l_module_serial_close(lua_State *L) /* optional explicit close */
{
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    if (u->port) {
        sp_close(u->port);
        sp_free_port(u->port);
        u->port = NULL;
    }
    lua_pushboolean(L, 1);
    return 1;
}
int l_gc(lua_State *L) { return l_module_serial_close(L); }

/*----------- registration ------------------------------------------*/
static const luaL_Reg port_mt[] = {{"__gc", l_gc}, {NULL, NULL}};

static const luaL_Reg module_fns[] = {
    {"get_port", l_module_serial_get_port_by_name},
    {"open", l_module_serial_open},
    {"close", l_module_serial_close},

    {"set_baudrate", l_module_serial_set_baudrate},
    {"set_bits", l_module_serial_set_bits},
    {"set_parity", l_module_serial_set_parity},
    {"set_stopbits", l_module_serial_set_stopbits},
    // {"set_rts", l_module_serial_set_rts},
    // {"set_cts", l_module_serial_set_cts},
    // {"set_dtr", l_module_serial_set_dtr},
    // {"set_dsr", l_module_serial_set_dsr},
    // {"set_xon_xoff", l_module_serial_set_xon_xoff},
    // {"set_flowcontrol", l_module_serial_set_flowcontrol},

    {"read_blocking", l_module_serial_read_blocking},
    {"read_nonblocking", l_module_serial_read_nonblocking},

    {"write_blocking", l_module_serial_write_blocking},
    {"write_nonblocking", l_module_serial_write_nonblocking},

    // {"get_input_waiting", l_module_serial_get_input_waiting},
    // {"get_output_waiting", l_module_serial_get_output_waiting},
    //
    // {"flush", l_module_serial_flush},
    // {"drain", l_module_serial_drain},
    {NULL, NULL}};

int l_module_serial_register_module(lua_State *L) {
    /* metatable for port userdata */
    luaL_newmetatable(L, "serial");
    luaL_setfuncs(L, port_mt, 0);
    lua_pop(L, 1);

    /* module table */
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    return 1;
}
