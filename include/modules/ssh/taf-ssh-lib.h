#ifndef MODULE_SSH2_LIB_H
#define MODULE_SSH2_LIB_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <libssh2.h>


int l_module_ssh_lib_init(lua_State *L);
int l_module_ssh_lib_exit(lua_State *L);
int l_module_ssh_lib_free(lua_State *L);
int l_module_ssh_lib_hostkey_hash(lua_State *L);

int l_module_ssh_banner_set(lua_State *L);
int l_module_ssh_base64_decode(lua_State *L);

int l_module_ssh_poll(lua_State *L);
int l_module_ssh_poll_chanell_read(lua_State *L);

int l_module_ssh_trace(lua_State *L);
int l_module_ssh_trace_sethandler(lua_State *L);

#endif // MODULE_SSH2_LIB_H
