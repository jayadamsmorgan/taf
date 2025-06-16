#ifndef MODULE_SSH2_SCP_H
#define MODULE_SSH2_SCP_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <libssh2.h>


int l_module_ssh_scp_recv(lua_State *L);
int l_module_ssh_scp_recv2(lua_State *L);
int l_module_ssh_scp_send(lua_State *L);
int l_module_ssh_scp_send64(lua_State *L);
int l_module_ssh_scp_send_ex(lua_State *L);

#endif // MODULE_SSH2_SCP_H
