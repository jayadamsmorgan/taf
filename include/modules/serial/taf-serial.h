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

// ts:close(port) -> string? err
int l_module_serial_close(lua_State *L);

// ts:drain(port:port_ud) -> string? err
int l_module_serial_drain(lua_State *L);

// ts:flush(port:port_ud, direction:string) -> string? err
int l_module_serial_flush(lua_State *L);

// ts:get_port(path:string) -> port_ud?, string? err
int l_module_serial_get_port_by_name(lua_State *L);

// ts:get_port_info(port:port_ud) -> port_info
int l_module_serial_get_port_info(lua_State *L);

// ts:get_waiting_input(port:port_ud) -> integer? bytes, string? err
int l_module_serial_get_input_waiting(lua_State *L);

// ts:get_waiting_output(port:port_ud) -> integer? bytes, string? err
int l_module_serial_get_output_waiting(lua_State *L);

// ts:list_devices() -> [port_info]?, string? err
int l_module_serial_list_ports(lua_State *L);

// ts:open(port:port_ud, mode:string) -> string? err
int l_module_serial_open_port(lua_State *L);

// ts:read_blocking(
//     port:port_ud,
//     byte_amout:integer,
//     timeout:integer=0
// ) -> string? read, string? err
int l_module_serial_read_blocking(lua_State *L);

// ts:read_nonblocking(
//     port:port_ud,
//     byte_amout:integer
// ) -> string? read, string? err
int l_module_serial_read_nonblocking(lua_State *L);

// ts:set_baudrate(port:port_ud, baudrate:number) -> string? err
int l_module_serial_set_baudrate(lua_State *L);

// ts:set_bits(port:port_ud, bits:number) -> string? err
int l_module_serial_set_bits(lua_State *L);

// ts:set_cts(port:port_ud, cts:string) -> string? err
int l_module_serial_set_cts(lua_State *L);

// ts:set_dsr(port:port_ud, dtr:string) -> string? err
int l_module_serial_set_dsr(lua_State *L);

// ts:set_dtr(port:port_ud, dtr:string) -> string? err
int l_module_serial_set_dtr(lua_State *L);

// ts:set_flowcontrol(port:port_ud, flowcontrol:string) -> string? err
int l_module_serial_set_flowcontrol(lua_State *L);

// ts:set_parity(port:port_ud, parity:string) -> string? err
int l_module_serial_set_parity(lua_State *L);

// ts:set_rts(port:port_ud, rts:string) -> string? err
int l_module_serial_set_rts(lua_State *L);

// ts:set_stopbits(port:port_ud, bits:integer) -> string? err
int l_module_serial_set_stopbits(lua_State *L);

// ts:set_xonxoff(port:port_ud, xonxoff:string) -> string? err
int l_module_serial_set_xon_xoff(lua_State *L);

// ts:write_blocking(
//     port:port_ud,
//     str_to_write:string,
//     (opt)timeout:integer=0
// ) -> integer? bytes_written, string? err
int l_module_serial_write_blocking(lua_State *L);

// ts:write_nonblocking(
//     port:port_ud,
//     str_to_write:string
// ) -> integer? bytes_written, string? err
int l_module_serial_write_nonblocking(lua_State *L);

/******************* API END *************************/

// Close every port opened by serialport library
void module_serial_close_all_ports();

// Register "taf-serial" module
int l_module_serial_register_module(lua_State *L);

#endif // MODULE_SERIAL_H
