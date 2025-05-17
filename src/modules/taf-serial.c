#include "modules/taf-serial.h"

#include <stdlib.h>
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
    return luaL_checkudata(L, idx, "taf-serial");
}

/* taf-serial:get_port(path:string) -> port_ud,nil | nil,err */
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

    luaL_getmetatable(L, "taf-serial");
    lua_setmetatable(L, -2);
    lua_pushnil(L);
    return 2;
}

typedef struct {
    char *path;
    char *description;
    char *serial;
    char *product;
    char *manufacturer;
    enum sp_transport type;
    int vid;
    int pid;
    int usb_addr;
    int usb_bus;
    char *bt_addr;
} port_info_helper_t;

static port_info_helper_t port_info_helper_from_port(struct sp_port *port) {
    port_info_helper_t pi = {0};

    char *desc = sp_get_port_description(port);
    pi.description = strdup(desc);

    char *path = sp_get_port_name(port);
    pi.path = strdup(path);

    enum sp_transport type = sp_get_port_transport(port);
    pi.type = type;

    if (type == SP_TRANSPORT_USB) {
        int usb_bus, usb_addr;
        enum sp_return r =
            sp_get_port_usb_bus_address(port, &usb_bus, &usb_addr);
        if (r == SP_OK) {
            pi.usb_bus = usb_bus;
            pi.usb_addr = usb_addr;
        }

        char *manufacturer = sp_get_port_usb_manufacturer(port);
        if (manufacturer && *manufacturer) {
            pi.manufacturer = strdup(manufacturer);
        }

        char *product = sp_get_port_usb_product(port);
        if (product && *product) {
            pi.product = strdup(product);
        }

        char *serial = sp_get_port_usb_serial(port);
        if (serial && *serial) {
            pi.serial = strdup(serial);
        }

        int vid, pid;
        r = sp_get_port_usb_vid_pid(port, &vid, &pid);
        if (r == SP_OK) {
            pi.vid = vid;
            pi.pid = pid;
        }
    }

    if (type == SP_TRANSPORT_BLUETOOTH) {
        char *bt_addr = sp_get_port_bluetooth_address(port);
        if (bt_addr && *bt_addr) {
            pi.bt_addr = strdup(bt_addr);
        }
    }

    return pi;
}

static void free_port_info_helper(port_info_helper_t *pi) {
    if (!pi) {
        return;
    }
    free(pi->serial);
    free(pi->product);
    free(pi->manufacturer);
    free(pi->bt_addr);
    free(pi->path);
    free(pi->description);
}

static const char *transport_str(enum sp_transport t) {
    switch (t) {
    case SP_TRANSPORT_NATIVE:
        return "native";
    case SP_TRANSPORT_USB:
        return "usb";
    case SP_TRANSPORT_BLUETOOTH:
        return "bluetooth";
    default:
        return "unknown";
    }
}

// @class port_info
// @field path string
// @filed type string
// @field description string
// @field serial string?
// @field product string?
// @field manufacturer string?
// @field vid number?
// @field pid number?
// @field usb_address number?
// @field usb_bus number?
// @field bluetooth_address string?

static void l_port_info_helper(lua_State *L, struct sp_port *port) {

    port_info_helper_t pi = port_info_helper_from_port(port);

    /* create one port_info table */
    lua_newtable(L);

    /* mandatory fields */
    lua_pushstring(L, pi.path);
    lua_setfield(L, -2, "path");
    lua_pushstring(L, transport_str(pi.type));
    lua_setfield(L, -2, "type");
    if (pi.description) {
        lua_pushstring(L, pi.description);
        lua_setfield(L, -2, "description");
    }

    /* optional USB-specific */
    if (pi.serial) {
        lua_pushstring(L, pi.serial);
        lua_setfield(L, -2, "serial");
    }
    if (pi.product) {
        lua_pushstring(L, pi.product);
        lua_setfield(L, -2, "product");
    }
    if (pi.manufacturer) {
        lua_pushstring(L, pi.manufacturer);
        lua_setfield(L, -2, "manufacturer");
    }
    if (pi.vid) {
        lua_pushinteger(L, pi.vid);
        lua_setfield(L, -2, "vid");
    }
    if (pi.pid) {
        lua_pushinteger(L, pi.pid);
        lua_setfield(L, -2, "pid");
    }
    if (pi.usb_addr) {
        lua_pushinteger(L, pi.usb_addr);
        lua_setfield(L, -2, "usb_address");
    }
    if (pi.usb_bus) {
        lua_pushinteger(L, pi.usb_bus);
        lua_setfield(L, -2, "usb_bus");
    }

    /* optional Bluetooth */
    if (pi.bt_addr) {
        lua_pushstring(L, pi.bt_addr);
        lua_setfield(L, -2, "bluetooth_address");
    }

    free_port_info_helper(&pi);
}

/* taf-serial:get_port_info(port:port_ud) -> port_info */
int l_module_serial_get_port_info(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    l_port_info_helper(L, u->port);

    return 1;
}

/* taf-serial:list_devices() -> table<port_info>, nil | nil, string */
int l_module_serial_list_ports(lua_State *L) {
    struct sp_port **ports;
    enum sp_return r = sp_list_ports(&ports);
    if (r != SP_OK) {
        lua_pushnil(L);
        lua_pushstring(L, sp_last_error_message());
        return 2;
    }

    lua_newtable(L); /* result list (idx = 1..) */
    int idx = 1;

    for (struct sp_port **p = ports; *p; ++p) {
        l_port_info_helper(L, *p);

        /* list[idx] = port_info */
        lua_rawseti(L, -2, idx++);
    }

    sp_free_port_list(ports);

    lua_pushnil(L); /* no error */
    return 2;       /* (result-table, nil) */
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

/* taf-serial:open(port:port_ud, mode:string) -> nil | err */
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

/* taf-serial:set_baudrate(port:port_ud, baudrate:integer) -> nil | err */
int l_module_serial_set_baudrate(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    enum sp_return r = (sp_set_baudrate(u->port, luaL_checkinteger(L, s + 1)));

    return push_result_nil(L, r);
}

/* taf-serial:set_bits(port:port_ud, bits:integer) -> nil | err */
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

/* taf-serial:set_parity(port:port_ud, parity:string) -> nil | err */
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

/* taf-serial:set_stopbits(port:port_ud, bits:integer) -> nil | err */
int l_module_serial_set_stopbits(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    enum sp_return r = (sp_set_stopbits(u->port, luaL_checkinteger(L, s + 1)));

    return push_result_nil(L, r);
}

static inline enum sp_rts sp_rts_from_str(const char *str) {
    if (!strcmp(str, "off"))
        return SP_RTS_OFF;
    if (!strcmp(str, "on"))
        return SP_RTS_ON;
    if (!strcmp(str, "flowctrl"))
        return SP_RTS_FLOW_CONTROL;
    return SP_RTS_INVALID;
}

/* taf-serial:set_rts(port:port_ud, rts:string) -> nil | err */
int l_module_serial_set_rts(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    enum sp_rts rts = sp_rts_from_str(luaL_checkstring(L, s + 1));
    if (rts < 0) {
        lua_pushstring(L, "invalid rts option, use 'off', 'on' or 'flowctrl'");
        return 1;
    }

    return push_result_nil(L, sp_set_rts(u->port, rts));
}

static inline enum sp_cts sp_cts_from_str(const char *str) {
    if (!strcmp(str, "ignore"))
        return SP_CTS_IGNORE;
    if (!strcmp(str, "flowctrl"))
        return SP_CTS_FLOW_CONTROL;
    return SP_CTS_INVALID;
}

/* taf-serial:set_cts(port:port_ud, cts:string) -> nil | err */
int l_module_serial_set_cts(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    enum sp_cts cts = sp_cts_from_str(luaL_checkstring(L, s + 1));
    if (cts < 0) {
        lua_pushstring(L, "invalid cts option, use 'ignore' or 'flowctrl'");
        return 1;
    }

    return push_result_nil(L, sp_set_cts(u->port, cts));
}

static inline enum sp_dtr sp_dtr_from_str(const char *str) {
    if (!strcmp(str, "off"))
        return SP_DTR_OFF;
    if (!strcmp(str, "on"))
        return SP_DTR_ON;
    if (!strcmp(str, "flowctrl"))
        return SP_DTR_FLOW_CONTROL;
    return SP_DTR_INVALID;
}

/* taf-serial:set_dtr(port:port_ud, dtr:string) -> nil | err */
int l_module_serial_set_dtr(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    enum sp_dtr dtr = sp_dtr_from_str(luaL_checkstring(L, s + 1));
    if (dtr < 0) {
        lua_pushstring(L, "invalid dtr option, use 'off', 'on' or 'flowctrl'");
        return 1;
    }

    return push_result_nil(L, sp_set_dtr(u->port, dtr));
}

static inline enum sp_dsr sp_dsr_from_str(const char *str) {
    if (!strcmp(str, "ignore"))
        return SP_DSR_IGNORE;
    if (!strcmp(str, "flowctrl"))
        return SP_DSR_FLOW_CONTROL;
    return SP_DSR_INVALID;
}

/* taf-serial:set_dsr(port:port_ud, dtr:string) -> nil | err */
int l_module_serial_set_dsr(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    enum sp_dsr dsr = sp_dsr_from_str(luaL_checkstring(L, s + 1));
    if (dsr < 0) {
        lua_pushstring(L, "invalid dsr option, use 'ignore' or 'flowctrl'");
        return 1;
    }

    return push_result_nil(L, sp_set_dsr(u->port, dsr));
}

static inline enum sp_xonxoff sp_xonxoff_from_str(const char *str) {
    if (!strcmp(str, "i"))
        return SP_XONXOFF_IN;
    if (!strcmp(str, "o"))
        return SP_XONXOFF_OUT;
    if (!strcmp(str, "io"))
        return SP_XONXOFF_INOUT;
    if (!strcmp(str, "disable"))
        return SP_XONXOFF_DISABLED;
    return SP_XONXOFF_INVALID;
}

/* taf-serial:set_xonxoff(port:port_ud, xonxoff:string) -> nil | err */
int l_module_serial_set_xon_xoff(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    enum sp_xonxoff xonxoff = sp_xonxoff_from_str(luaL_checkstring(L, s + 1));
    if (xonxoff < 0) {
        lua_pushstring(
            L, "invalid xonxoff option, use 'i', 'o', 'io' or 'disable'");
        return 1;
    }

    return push_result_nil(L, sp_set_xon_xoff(u->port, xonxoff));
}

static inline enum sp_flowcontrol sp_flowcontrol_from_str(const char *str) {
    if (!strcmp(str, "dtrdsr"))
        return SP_FLOWCONTROL_DTRDSR;
    if (!strcmp(str, "rtscts"))
        return SP_FLOWCONTROL_RTSCTS;
    if (!strcmp(str, "xonxoff"))
        return SP_FLOWCONTROL_XONXOFF;
    if (!strcmp(str, "none"))
        return SP_FLOWCONTROL_NONE;
    return -1;
}

/* taf-serial:set_flowcontrol(port:port_ud, flowcontrol:string) -> nil | err */
int l_module_serial_set_flowcontrol(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    enum sp_flowcontrol fc =
        sp_flowcontrol_from_str(luaL_checkstring(L, s + 1));
    if (fc < 0) {
        lua_pushstring(L, "invalid flowcontrol option, use 'dtrdsr', 'rtscts', "
                          "'xonxoff' or 'none'");
        return 1;
    }

    return push_result_nil(L, sp_set_flowcontrol(u->port, fc));
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

/* taf-serial:read_blocking(port:port_ud, byte_amout:integer,
 * (opt)timeout:integer=0)
 * -> string, nil | nil, err */
int l_module_serial_read_blocking(lua_State *L) { return read_helper(L, 1); }

/* taf-serial:read_nonblocking(port:port_ud, byte_amout:integer)
 * -> string, nil | nil, err */
int l_module_serial_read_nonblocking(lua_State *L) { return read_helper(L, 0); }

/*----------- writing ------------------------------------------------*/
static inline int write_helper(lua_State *L, int blocking) {
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

/* taf-serial:write_blocking(port:port_ud, string, (opt)timeout:integer=0)
 * -> integer, nil | nil, err */
int l_module_serial_write_blocking(lua_State *L) { return write_helper(L, 1); }

/* taf-serial:write_nonblocking(port:port_ud, string) -> integer, nil | nil, err
 */
int l_module_serial_write_nonblocking(lua_State *L) {
    return write_helper(L, 0);
}

/*----------- status -------------------------------------------------*/
static inline int status_helper(lua_State *L, int direction) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    enum sp_return r =
        direction ? sp_input_waiting(u->port) : sp_output_waiting(u->port);
    if (r < SP_OK) {
        lua_pushnil(L);
        lua_pushstring(L, sp_last_error_message());
        return 2;
    }
    lua_pushinteger(L, r);
    lua_pushnil(L);

    return 2;
}

/* taf-serial:get_waiting_input(port:port_ud) -> integer, nil | nil, err */
int l_module_serial_get_input_waiting(lua_State *L) {
    return status_helper(L, 1);
}

/* taf-serial:get_waiting_output(port:port_ud) -> integer, nil | nil, err */
int l_module_serial_get_output_waiting(lua_State *L) {
    return status_helper(L, 0);
}

/*----------- flush / drain -----------------------------------------*/

static inline enum sp_buffer sp_buffer_from_str(const char *str) {
    if (!strcmp(str, "i"))
        return SP_BUF_INPUT;
    if (!strcmp(str, "o"))
        return SP_BUF_OUTPUT;
    if (!strcmp(str, "io"))
        return SP_BUF_BOTH;
    return -1;
}

/* taf-serial:flush(port:port_ud, direction:string) -> nil|string */
int l_module_serial_flush(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    const char *mode = luaL_checkstring(L, s + 1);
    enum sp_buffer dir = sp_buffer_from_str(mode);
    if (dir < 0) {
        lua_pushstring(L, "invalid direction, use 'i', 'o' or 'io'");
        return 1;
    }

    return push_result_nil(L, sp_flush(u->port, dir));
}

/* taf-serial:drain(port:port_ud) -> nil|string */
int l_module_serial_drain(lua_State *L) {
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    return push_result_nil(L, sp_drain(u->port));
}

/*----------- close / GC --------------------------------------------*/
/* taf-serial:close(port) -> nil|string */
int l_module_serial_close(lua_State *L) /* optional explicit close */
{
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    if (u->port) {
        enum sp_return r = sp_close(u->port);
        if (r != SP_OK) {
            lua_pushstring(L, sp_last_error_message());
            return 1;
        }
        sp_free_port(u->port);
        u->port = NULL;
    }
    lua_pushnil(L);
    return 1;
}

// FIXME
//
// it would probably be a good idea to keep track of allocated ports
// and then free and close them all here
static int l_gc(lua_State *L) { return l_module_serial_close(L); }

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
    {"set_rts", l_module_serial_set_rts},
    {"set_cts", l_module_serial_set_cts},
    {"set_dtr", l_module_serial_set_dtr},
    {"set_dsr", l_module_serial_set_dsr},
    {"set_xon_xoff", l_module_serial_set_xon_xoff},
    {"set_flowcontrol", l_module_serial_set_flowcontrol},

    {"read_blocking", l_module_serial_read_blocking},
    {"read_nonblocking", l_module_serial_read_nonblocking},

    {"write_blocking", l_module_serial_write_blocking},
    {"write_nonblocking", l_module_serial_write_nonblocking},

    {"get_waiting_input", l_module_serial_get_input_waiting},
    {"get_waiting_output", l_module_serial_get_output_waiting},

    {"get_port_info", l_module_serial_get_port_info},
    {"list_devices", l_module_serial_list_ports},

    {"flush", l_module_serial_flush},
    {"drain", l_module_serial_drain},
    {NULL, NULL}};

int l_module_serial_register_module(lua_State *L) {
    /* metatable for port userdata */
    luaL_newmetatable(L, "taf-serial");
    luaL_setfuncs(L, port_mt, 0);
    lua_pop(L, 1);

    /* module table */
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    return 1;
}
