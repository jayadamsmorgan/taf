#ifndef MODULE_SERIAL_H
#define MODULE_SERIAL_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <libserialport.h>

typedef struct {
    struct sp_port *port;
} l_module_serial_t;

/******************* API START ***********************/

// port_info:
// - path: string
// - type: string
// - description: string
// - serial: string?
// - product: string?
// - manufacturer: string?
// - vid: number?
// - pid: number?
// - usb_address: number?
// - usb_bus: number?
// - bluetooth_address: string?

// port:close(self:port)
int l_module_serial_close(lua_State *L);

// port:drain(self:port)
int l_module_serial_drain(lua_State *L);

// port:flush(self:port, direction:string)
int l_module_serial_flush(lua_State *L);

// ts:get_port(path:string) -> port
int l_module_serial_get_port_by_name(lua_State *L);

// port:get_port_info(self:port) -> port_info
int l_module_serial_get_port_info(lua_State *L);

// port:get_waiting_input(self:port) -> integer bytes
int l_module_serial_get_input_waiting(lua_State *L);

// port:get_waiting_output(self:port) -> integer bytes
int l_module_serial_get_output_waiting(lua_State *L);

// ts:list_devices() -> [port_info]
int l_module_serial_list_ports(lua_State *L);

// port:open(self:port, mode:string)
int l_module_serial_open_port(lua_State *L);

// port:read_blocking(
//     self:port,
//     byte_amout:integer,
//     timeout:integer=0
// ) -> string read
int l_module_serial_read_blocking(lua_State *L);

// port:read_nonblocking(
//     self:port,
//     byte_amout:integer
// ) -> string read
int l_module_serial_read_nonblocking(lua_State *L);

// port:set_baudrate(self:port, baudrate:integer)
int l_module_serial_set_baudrate(lua_State *L);

// port:set_bits(self:port, bits:integer)
int l_module_serial_set_bits(lua_State *L);

// port:set_cts(self:port, cts:string)
int l_module_serial_set_cts(lua_State *L);

// port:set_dsr(self:port, dtr:string)
int l_module_serial_set_dsr(lua_State *L);

// port:set_dtr(self:port, dtr:string)
int l_module_serial_set_dtr(lua_State *L);

// port:set_flowcontrol(self:port, flowcontrol:string)
int l_module_serial_set_flowcontrol(lua_State *L);

// port:set_parity(self:port, parity:string)
int l_module_serial_set_parity(lua_State *L);

// port:set_rts(self:port, rts:string)
int l_module_serial_set_rts(lua_State *L);

// port:set_stopbits(self:port, bits:integer)
int l_module_serial_set_stopbits(lua_State *L);

// port:set_xonxoff(self:port, xonxoff:string)
int l_module_serial_set_xon_xoff(lua_State *L);

// port:write_blocking(
//     self:port,
//     str_to_write:string,
//     (opt)timeout:integer=0
// ) -> integer bytes_written
int l_module_serial_write_blocking(lua_State *L);

// port:write_nonblocking(
//     self:port,
//     str_to_write:string
// ) -> integer bytes_written
int l_module_serial_write_nonblocking(lua_State *L);

/******************* API END *************************/

// Register "taf-serial" module
int l_module_serial_register_module(lua_State *L);

#endif // MODULE_SERIAL_H
