#ifndef MODULE_SERIAL_H
#define MODULE_SERIAL_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <libserialport.h>

typedef struct {
    struct sp_port *port;
} l_module_serial_t;

int l_module_serial_register_module(lua_State *L);

int l_module_serial_get_port_by_name(lua_State *L);
int l_module_serial_list_ports(lua_State *L);
int l_module_serial_copy_port(lua_State *L);
int l_module_serial_open_port(lua_State *L);

int l_module_serial_set_baudrate(lua_State *L);
int l_module_serial_set_bits(lua_State *L);
int l_module_serial_set_parity(lua_State *L);
int l_module_serial_set_stopbits(lua_State *L);

int l_module_serial_set_rts(lua_State *L);
int l_module_serial_set_cts(lua_State *L);
int l_module_serial_set_dtr(lua_State *L);
int l_module_serial_set_dsr(lua_State *L);
int l_module_serial_set_xon_xoff(lua_State *L);
int l_module_serial_set_flowcontrol(lua_State *L);

int l_module_serial_read_blocking(lua_State *L);
int l_module_serial_read_nonblocking(lua_State *L);

int l_module_serial_write_blocking(lua_State *L);
int l_module_serial_write_nonblocking(lua_State *L);

int l_module_serial_get_input_waiting(lua_State *L);
int l_module_serial_get_output_waiting(lua_State *L);

int l_module_serial_flush(lua_State *L);
int l_module_serial_drain(lua_State *L);

#endif // MODULE_SERIAL_H
