#include "modules/serial/taf-serial.h"

#include "internal_logging.h"
#include "util/lua.h"

#include <stdlib.h>
#include <string.h>

static inline l_module_serial_t *check_port(lua_State *L, int idx) {
    return luaL_checkudata(L, idx, "taf-serial");
}

int l_module_serial_get_port_by_name(lua_State *L) {
    LOG("Invoked taf-serial get_port...");
    int s = selfshift(L);
    const char *p = luaL_checkstring(L, s);

    l_module_serial_t *u = lua_newuserdata(L, sizeof *u);
    u->port = NULL;

    enum sp_return r = sp_get_port_by_name(p, &u->port);

    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("SP error: %s", err);
        return luaL_error(L, err);
    }

    luaL_getmetatable(L, "taf-serial");
    lua_setmetatable(L, -2);

    LOG("Successfully finished taf-serial get_port.");
    return 1;
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
    if (desc) {
        LOG("Got port description %s", desc);
        pi.description = strdup(desc);
    }

    char *path = sp_get_port_name(port);
    if (path) {
        LOG("Got port path %s", path);
        pi.path = strdup(path);
    }

    enum sp_transport type = sp_get_port_transport(port);
    LOG("Port transport type: %d", type);
    pi.type = type;

    if (type == SP_TRANSPORT_USB) {
        int usb_bus, usb_addr;
        enum sp_return r =
            sp_get_port_usb_bus_address(port, &usb_bus, &usb_addr);
        if (r == SP_OK) {
            LOG("Got port USB Bus %d, Address %d", usb_bus, usb_addr);
            pi.usb_bus = usb_bus;
            pi.usb_addr = usb_addr;
        }

        char *manufacturer = sp_get_port_usb_manufacturer(port);
        if (manufacturer) {
            LOG("Got port manufacturer: %s", manufacturer);
            pi.manufacturer = strdup(manufacturer);
        }

        char *product = sp_get_port_usb_product(port);
        if (product) {
            LOG("Got port product: %s", product);
            pi.product = strdup(product);
        }

        char *serial = sp_get_port_usb_serial(port);
        if (serial) {
            LOG("Got port serial: %s", serial);
            pi.serial = strdup(serial);
        }

        int vid, pid;
        r = sp_get_port_usb_vid_pid(port, &vid, &pid);
        if (r == SP_OK) {
            LOG("Got port VendorID %d, ProductID %d", vid, pid);
            pi.vid = vid;
            pi.pid = pid;
        }
    }

    if (type == SP_TRANSPORT_BLUETOOTH) {
        char *bt_addr = sp_get_port_bluetooth_address(port);
        if (bt_addr) {
            LOG("Got port Bluetooth Address %s", bt_addr);
            pi.bt_addr = strdup(bt_addr);
        }
    }

    return pi;
}

static void free_port_info_helper(port_info_helper_t *pi) {
    LOG("Freeing port info helper...");
    if (!pi) {
        return;
    }
    free(pi->serial);
    free(pi->product);
    free(pi->manufacturer);
    free(pi->bt_addr);
    free(pi->path);
    free(pi->description);
    LOG("Port info helper freed.");
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

static void l_port_info_helper(lua_State *L, struct sp_port *port) {

    LOG("Getting port info helper for port (pointer %p)", (void *)port);

    port_info_helper_t pi = port_info_helper_from_port(port);

    /* create one port_info table */
    lua_newtable(L);

    LOG("Pushing port info fields into Lua table...");

    lua_pushstring(L, transport_str(pi.type));
    lua_setfield(L, -2, "type");
    if (pi.path) {
        lua_pushstring(L, pi.path);
        lua_setfield(L, -2, "path");
    }
    if (pi.description) {
        lua_pushstring(L, pi.description);
        lua_setfield(L, -2, "description");
    }

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

    if (pi.bt_addr) {
        lua_pushstring(L, pi.bt_addr);
        lua_setfield(L, -2, "bluetooth_address");
    }

    free_port_info_helper(&pi);

    LOG("Successfully got port info.");
}

int l_module_serial_get_port_info(lua_State *L) {

    LOG("Invoked taf-serial get_port_info...");

    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    LOG("Got port pointer %p", (void *)u);

    l_port_info_helper(L, u->port);

    LOG("Successfully finished taf-serial get_port_info.");

    return 1;
}

int l_module_serial_list_ports(lua_State *L) {

    LOG("Invoked taf-serial list_ports...");

    struct sp_port **ports;
    enum sp_return r = sp_list_ports(&ports);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Could not get serial ports: %s", err);
        return luaL_error(L, err);
    }

    LOG("Creating result list...");
    lua_newtable(L);
    int idx = 1;

    for (struct sp_port **p = ports; *p; ++p) {
        l_port_info_helper(L, *p);

        lua_rawseti(L, -2, idx++);
    }

    LOG("Freeing serial port list...");
    sp_free_port_list(ports);

    LOG("Successfully finished taf-serial list_ports");
    return 1;
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

int l_module_serial_open(lua_State *L) {

    LOG("Invoked taf-serial open...");

    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    const char *mode_str = luaL_checkstring(L, s + 1);
    int mode = mode_from_str(mode_str);
    if (mode == -1) {
        return luaL_error(L, "invalid mode, use 'r', 'w' or 'rw'");
    }

    enum sp_return r = sp_open(u->port, mode);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to open port: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial open.");
    return 0;
}

int l_module_serial_set_baudrate(lua_State *L) {
    LOG("Invoked taf-serial set_baudrate...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    enum sp_return r = (sp_set_baudrate(u->port, luaL_checkinteger(L, s + 1)));
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to set baudrate: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial set_baudrate.");
    return 0;
}

int l_module_serial_set_bits(lua_State *L) {
    LOG("Invoked taf-serial set_bits...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    enum sp_return r = (sp_set_bits(u->port, luaL_checkinteger(L, s + 1)));
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to set bits: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial set_bits.");
    return 0;
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
    return -1;
}

int l_module_serial_set_parity(lua_State *L) {
    LOG("Invoked taf-serial set_parity...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    const char *str = luaL_checkstring(L, s + 1);
    int par = parity_from_str(str);
    if (par == -1) {
        return luaL_error(
            L, "invalid parity, use 'none', 'odd', 'even', 'mark' or 'space'");
    }

    enum sp_return r = sp_set_parity(u->port, par);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to set parity: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial set_parity.");
    return 0;
}

int l_module_serial_set_stopbits(lua_State *L) {
    LOG("Invoked taf-serial set_stopbits...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    enum sp_return r = (sp_set_stopbits(u->port, luaL_checkinteger(L, s + 1)));
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to set parity: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial set_stopbits.");
    return 0;
}

static inline enum sp_rts sp_rts_from_str(const char *str) {
    if (!strcmp(str, "off"))
        return SP_RTS_OFF;
    if (!strcmp(str, "on"))
        return SP_RTS_ON;
    if (!strcmp(str, "flowctrl"))
        return SP_RTS_FLOW_CONTROL;
    return -1;
}

int l_module_serial_set_rts(lua_State *L) {
    LOG("Invoked taf-serial set_rts...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    int rts = sp_rts_from_str(luaL_checkstring(L, s + 1));
    if (rts == -1) {
        return luaL_error(L,
                          "invalid rts option, use 'off', 'on' or 'flowctrl'");
    }

    enum sp_return r = sp_set_rts(u->port, rts);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to set RTS: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial set_rts.");
    return 0;
}

static inline enum sp_cts sp_cts_from_str(const char *str) {
    if (!strcmp(str, "ignore"))
        return SP_CTS_IGNORE;
    if (!strcmp(str, "flowctrl"))
        return SP_CTS_FLOW_CONTROL;
    return -1;
}

int l_module_serial_set_cts(lua_State *L) {
    LOG("Invoked taf-serial set_cts...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    int cts = sp_cts_from_str(luaL_checkstring(L, s + 1));
    if (cts == -1) {
        return luaL_error(L, "invalid cts option, use 'ignore' or 'flowctrl'");
    }

    enum sp_return r = sp_set_cts(u->port, cts);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to set CTS: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial set_cts.");
    return 0;
}

static inline enum sp_dtr sp_dtr_from_str(const char *str) {
    if (!strcmp(str, "off"))
        return SP_DTR_OFF;
    if (!strcmp(str, "on"))
        return SP_DTR_ON;
    if (!strcmp(str, "flowctrl"))
        return SP_DTR_FLOW_CONTROL;
    return -1;
}

int l_module_serial_set_dtr(lua_State *L) {
    LOG("Invoked taf-serial set_dtr...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    int dtr = sp_dtr_from_str(luaL_checkstring(L, s + 1));
    if (dtr == -1) {
        return luaL_error(L,
                          "invalid dtr option, use 'off', 'on' or 'flowctrl'");
    }

    enum sp_return r = sp_set_dtr(u->port, dtr);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to set DTR: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial set_dtr.");
    return 0;
}

static inline enum sp_dsr sp_dsr_from_str(const char *str) {
    if (!strcmp(str, "ignore"))
        return SP_DSR_IGNORE;
    if (!strcmp(str, "flowctrl"))
        return SP_DSR_FLOW_CONTROL;
    return -1;
}

int l_module_serial_set_dsr(lua_State *L) {
    LOG("Invoked taf-serial set_dsr...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    int dsr = sp_dsr_from_str(luaL_checkstring(L, s + 1));
    if (dsr == -1) {
        return luaL_error(L, "invalid dsr option, use 'ignore' or 'flowctrl'");
    }

    enum sp_return r = sp_set_dsr(u->port, dsr);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to set DSR: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial set_dsr.");
    return 0;
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
    return -1;
}

int l_module_serial_set_xon_xoff(lua_State *L) {
    LOG("Invoked taf-serial set_xon_xoff...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    int xonxoff = sp_xonxoff_from_str(luaL_checkstring(L, s + 1));
    if (xonxoff == -1) {
        return luaL_error(
            L, "invalid xonxoff option, use 'i', 'o', 'io' or 'disable'");
    }

    enum sp_return r = sp_set_xon_xoff(u->port, xonxoff);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to set XON/XOFF: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial set_xon_xoff.");
    return 0;
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

int l_module_serial_set_flowcontrol(lua_State *L) {
    LOG("Invoked taf-serial set_flowcontrol...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    int fc = sp_flowcontrol_from_str(luaL_checkstring(L, s + 1));
    if (fc == -1) {
        return luaL_error(L,
                          "invalid flowcontrol option, use 'dtrdsr', 'rtscts', "
                          "'xonxoff' or 'none'");
    }

    enum sp_return r = sp_set_flowcontrol(u->port, fc);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to set flowcontrol: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial set_flowcontrol.");
    return 0;
}

/*----------- reading ------------------------------------------------*/
static inline int read_helper(lua_State *L, int blocking) {
    LOG("Invoked taf-serial read. Blocking: %d", blocking);
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    int n = luaL_checkinteger(L, s + 1);
    int to_ms = luaL_optinteger(L, s + 2, 0);
    LOG("Amount of bytes to read: %d, timeout: %d", n, to_ms);

    luaL_Buffer b;
    char *buf = luaL_buffinitsize(L, &b, n);
    int got = blocking ? sp_blocking_read(u->port, buf, n, to_ms)
                       : sp_nonblocking_read(u->port, buf, n);

    if (got < 0) {
        const char *err = sp_last_error_message();
        LOG("Unable to read: %s", err);
        return luaL_error(L, err);
    }

    LOG("Read %d bytes: %.*s", got, got, buf);
    luaL_addsize(&b, got);
    luaL_pushresult(&b);

    LOG("Successfully finished taf-serial read.");
    return 1;
}

int l_module_serial_read_blocking(lua_State *L) { return read_helper(L, 1); }

int l_module_serial_read_nonblocking(lua_State *L) { return read_helper(L, 0); }

/*----------- writing ------------------------------------------------*/
static inline int write_helper(lua_State *L, int blocking) {
    LOG("Invoked taf-serial write. Blocking: %d", blocking);
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    size_t len;
    const char *buf = luaL_checklstring(L, s + 1, &len);
    int to_ms = luaL_optinteger(L, s + 2, 0);
    LOG("Length: %zu, Buffer: '%.*s', timeout: %d", len, (int)len, buf, to_ms);

    int wrote = blocking ? sp_blocking_write(u->port, buf, len, to_ms)
                         : sp_nonblocking_write(u->port, buf, len);

    if (wrote < 0) {
        const char *err = sp_last_error_message();
        LOG("Unable to write: %s", err);
        return luaL_error(L, err);
    }
    LOG("Wrote %d bytes.", wrote);

    lua_pushinteger(L, wrote);

    LOG("Successfully finished taf-serial write.");
    return 1;
}

int l_module_serial_write_blocking(lua_State *L) { return write_helper(L, 1); }

int l_module_serial_write_nonblocking(lua_State *L) {
    return write_helper(L, 0);
}

/*----------- status -------------------------------------------------*/
static inline int status_helper(lua_State *L, int direction) {
    LOG("Invoked taf-serial get_waiting, direction: %d...", direction);
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    enum sp_return r =
        direction ? sp_input_waiting(u->port) : sp_output_waiting(u->port);
    if (r < SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to get_waiting: %s", err);
        return luaL_error(L, err);
    }
    LOG("Waiting: %d", r);
    lua_pushinteger(L, r);

    LOG("Successfully finished taf-serial get_waiting.");

    return 1;
}

int l_module_serial_get_input_waiting(lua_State *L) {
    return status_helper(L, 1);
}

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

int l_module_serial_flush(lua_State *L) {
    LOG("Invoked taf-serial flush...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    const char *mode = luaL_checkstring(L, s + 1);
    int dir = sp_buffer_from_str(mode);
    LOG("Direction: %d", dir);
    if (dir == -1) {
        return luaL_error(L, "invalid direction, use 'i', 'o' or 'io'");
    }

    enum sp_return r = sp_flush(u->port, dir);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to flush: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial flush.");
    return 0;
}

int l_module_serial_drain(lua_State *L) {
    LOG("Invoked taf-serial drain...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);

    enum sp_return r = sp_drain(u->port);
    if (r != SP_OK) {
        const char *err = sp_last_error_message();
        LOG("Unable to drain: %s", err);
        return luaL_error(L, err);
    }

    LOG("Successfully finished taf-serial drain.");
    return 0;
}

int l_module_serial_close(lua_State *L) {
    LOG("Invoked taf-serial close...");
    int s = selfshift(L);
    l_module_serial_t *u = check_port(L, s);
    if (u->port) {
        enum sp_return r = sp_close(u->port);
        if (r != SP_OK) {
            const char *err = sp_last_error_message();
            LOG("Unable to close: %s", err);
            return luaL_error(L, err);
        }
        LOG("Freeing port...");
        sp_free_port(u->port);
        u->port = NULL;
    } else {
        LOG("Port is NULL.");
    }
    LOG("Successfully finished taf-serial close.");
    return 0;
}

static int l_gc(lua_State *L) {
    LOG("Invoked taf-serial port GC...");
    return l_module_serial_close(L);
}

/*----------- registration ------------------------------------------*/
static const luaL_Reg port_mt[] = {
    {"close", l_module_serial_close},
    {"drain", l_module_serial_drain},
    {"flush", l_module_serial_flush},
    {"get_port_info", l_module_serial_get_port_info},
    {"get_waiting_input", l_module_serial_get_input_waiting},
    {"get_waiting_output", l_module_serial_get_output_waiting},
    {"open", l_module_serial_open},
    {"read_blocking", l_module_serial_read_blocking},
    {"read_nonblocking", l_module_serial_read_nonblocking},
    {"set_baudrate", l_module_serial_set_baudrate},
    {"set_bits", l_module_serial_set_bits},
    {"set_cts", l_module_serial_set_cts},
    {"set_dsr", l_module_serial_set_dsr},
    {"set_dtr", l_module_serial_set_dtr},
    {"set_flowcontrol", l_module_serial_set_flowcontrol},
    {"set_parity", l_module_serial_set_parity},
    {"set_rts", l_module_serial_set_rts},
    {"set_stopbits", l_module_serial_set_stopbits},
    {"set_xon_xoff", l_module_serial_set_xon_xoff},
    {"write_blocking", l_module_serial_write_blocking},
    {"write_nonblocking", l_module_serial_write_nonblocking},
    {"__gc", l_gc},
    {NULL, NULL}};

static const luaL_Reg module_fns[] = {
    {"get_port", l_module_serial_get_port_by_name},
    {"list_devices", l_module_serial_list_ports},
    {NULL, NULL},
};

int l_module_serial_register_module(lua_State *L) {
    LOG("Registering taf-serial module...");

    luaL_newmetatable(L, "taf-serial");

    LOG("Registering port functions...");
    lua_newtable(L);
    luaL_setfuncs(L, port_mt, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    LOG("Port functions registered.");

    LOG("Registering module functions...");
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    LOG("Module functions registered.");

    LOG("Successfully registered taf-serial module.");
    return 1;
}
